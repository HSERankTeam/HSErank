#include <algorithm>
#include <bits/chrono.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <iostream>

#include "userver/engine/sleep.hpp"
#include "userver/logging/log.hpp"
#include "userver/storages/postgres/cluster_types.hpp"
#include "userver/storages/postgres/exceptions.hpp"
#include "userver/storages/postgres/options.hpp"
#include "userver/storages/postgres/postgres_fwd.hpp"
#include "userver/storages/postgres/query.hpp"
#include "userver/testsuite/testsuite_support.hpp"
#include <userver/server/handlers/ping.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/engine/sleep.hpp>

#include <userver/engine/deadline.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include <zconf.h>
#include <zlib.h>

#include "userver/utils/periodic_task.hpp"
#include "xml_dblp.h"


namespace dblpservice {

class UpdateDB final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-updatedb";
    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequestThrow(
        const server::http::HttpRequest& request,
        server::request::RequestContext&) const override {
        return "UPDATE NOT NEED";
    }

    void task() {
        const auto response = httpClient_.CreateNotSignedRequest()
            -> get("http://185.188.183.91/dblp.xml.gz.md5")
            -> timeout(std::chrono::seconds(15))
            -> retry()
            -> perform();
        if (!response->IsOk()) {
            LOG_ERROR() << response->body() << "dblp return status: " << response->status_code();
        }

        std::string_view availableDataHash = response->body_view();
        
        bool newDataAvailable = true;

        try {
            constexpr auto hashQ = R"~(
                SELECT param_value FROM var WHERE param_name = 'last_update_hash'
            )~";
            auto res = pgCluster_->Execute(storages::postgres::ClusterHostType::kMaster, hashQ).AsSingleRow<std::string>();
            LOG_INFO() << res;
            if (res == availableDataHash) {
                newDataAvailable = false;
            }
        } catch(storages::postgres::Error& e) {
            LOG_ERROR() << "hash check fail " << e.what();
        }

        if (newDataAvailable) {
            // get length of content
            const auto response = httpClient_.CreateRequest()
                -> head("http://185.188.183.91/dblp.xml.gz")
                -> timeout(std::chrono::seconds(15))
                -> retry()
                -> perform();

            uint64_t length;
            try {
                length = std::stoull(response->headers().at("Content-Length"));
            } catch (...) {
                LOG_ERROR() << "invalid Content-Length: " << length;
                throw;
            }

            z_stream stream = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            int result = inflateInit2(&stream, MAX_WBITS + 16);
            if (result != Z_OK) {
                LOG_ERROR() << "cannot initialize zstream";
                throw std::runtime_error("");
            }
            // LOG_DEBUG() << "LENGTH " << length; 
            dblpXmlArticleParser parser({"article"});
            for (size_t i = 0; i < length; i += downloadPartSize) {
                // LOG_ERROR() << double(i) / length * 100 << " PERCENTS ";
                auto from = std::to_string(i);
                auto to = std::to_string(std::min(i + downloadPartSize - 1, length - 1));
                const auto response = httpClient_.CreateRequest()
                // TODO move to config
                -> get("http://185.188.183.91/dblp.xml.gz")
                -> timeout(std::chrono::seconds(15))
                -> retry(10)
                -> headers({{"Range", "bytes="+from+"-"+to}})
                -> perform();

                if (response->status_code() != 206 && response->status_code() != 200) {
                    // TODO wait & retry if 429 code
                    LOG_ERROR() << "partial download failed " << response->status_code() << ' ' << response->body_view() << ' ' << from << ' ' << to;
                    // if (response->status_code() == 429) {
                    //     engine::SleepFor(std::chrono::milliseconds{50});
                    //     continue;
                    // }
                    throw std::runtime_error("");
                }

                if (response->body_view().size() > downloadPartSize) {
                    LOG_ERROR() << "partial download failed: recived > requested " << response->body_view().size() << ' ' << downloadPartSize;
                    throw std::runtime_error("");
                }

                uint8_t inbuff[downloadPartSize];
                uint8_t outbuff[outBufferSize];

                response->body_view().copy(reinterpret_cast<char*>(inbuff), response->body_view().size());


                stream.avail_in = response->body_view().size();
                stream.next_in = inbuff;
                
                int status{};
                size_t writed{};

                do {
                    stream.avail_out = outBufferSize;
                    stream.next_out = outbuff;
                    status = inflate(&stream, Z_SYNC_FLUSH);
                    if (status == Z_NEED_DICT) {
                        // break;
                        throw std::runtime_error("Z_NEED_DICT");
                    }
                    if (status == Z_DATA_ERROR) {
                        // break;
                        throw std::runtime_error("Z_DATA_ERROR");
                    }
                    if (status == Z_STREAM_ERROR) {
                        // break;
                        throw std::runtime_error("Z_STREAM_ERROR");
                    }
                    if (status == Z_MEM_ERROR) {
                        // break;
                        throw std::runtime_error("Z_MEM_ERROR");
                    }
                    if (status == Z_BUF_ERROR) {
                        throw std::runtime_error("Z_BUF_ERROR");
                        // break;
                    }
                    LOG_INFO() << "OK: " << status << ' ' << stream.avail_in << ' ' << stream.avail_out;
                    writed = outBufferSize - stream.avail_out;
                    // TODO remove string alloc
                    parser.write(std::string(outbuff, outbuff + writed));
                    // LOG_DEBUG() << "PARSED:" << std::string(outbuff, outbuff + writed);
                    auto articles = parser.get();
                    // LOG_DEBUG() << "SENDING";
                    sendArticlesToDb(articles);
                } while(0 < stream.avail_in);
            }
            if (inflateEnd(&stream) != Z_OK) {
                throw std::runtime_error("failed inflateEnd");
            }
            {
                constexpr auto updateHashQ = R"~(
                    UPDATE var SET
                        param_value = {}
                    WHERE param_name = 'last_update_hash'
                )~";
                pgCluster_->Execute(storages::postgres::ClusterHostType::kMaster, updateHashQ, availableDataHash);
            }
        }
    }

    void sendArticlesToDb(const std::vector<articlePart>& data) const {
        if (data.empty()) return;
        std::string request;
        size_t cnt = 0;
        request += "INSERT INTO articles VALUES ";
        std::vector<std::string_view> uniqueAuthors;
        for (const auto& article : data) {
            if (article.author.size() > 10) {
                continue;
            }
            for (const auto& author : article.author) {
                uniqueAuthors.emplace_back(author);
                // TODO format string
                request += "(";
                append(request, article.title);
                request += ",";
                append(request, author);
                request += ",";
                append(request, article.journal);
                request += ",";
                // append(request, article.year);
                request += article.year;
                request += ')';
                request += ',';
                ++cnt;
            }
        }
        if (cnt == 0) {
            return;
        }
        request.pop_back();
        request += R"~(
            ON CONFLICT (title, author) DO NOTHING
        )~";

        std::sort(uniqueAuthors.begin(), uniqueAuthors.end());
        uniqueAuthors.resize(std::unique(uniqueAuthors.begin(), uniqueAuthors.end()) - uniqueAuthors.begin());
        std::string authorsRequest;
        authorsRequest += R"~(
            INSERT into authors (name) VALUES
        )~";
        for (auto& author : uniqueAuthors) {
            authorsRequest += '(';
            append(authorsRequest, author);
            authorsRequest += ')';
            authorsRequest += ',';
        }
        authorsRequest.pop_back();
        authorsRequest += R"~(
            ON CONFLICT (name) DO NOTHING
        )~";


        storages::postgres::OptionalCommandControl timeouts = storages::postgres::CommandControl(
            std::chrono::seconds{200}, 
            std::chrono::seconds{100}
        );

        storages::postgres::TransactionOptions options;

        auto trx = pgCluster_->Begin(options, timeouts);
        
        using storages::postgres::ClusterHostType;

        try {
            auto result1 = trx.Execute(authorsRequest);
            auto result2 = trx.Execute(request);
            trx.Commit();
        } catch(storages::postgres::Error& e) {
            LOG_ERROR() << e.what() << ' ' << data.size() << request;
            trx.Rollback();
        }
    }

    static void append(std::string& request, std::string_view data) {
        request += '\'';
        {
            for (char c : data) {
                if (c == '\'') {
                    request += '\'';
                }
                request += c;
            }
        }
        request += '\'';
    }

    UpdateDB(const components::ComponentConfig& config,
            const components::ComponentContext& context):
        server::handlers::HttpHandlerBase(config, context),
        httpClient_(context.FindComponent<components::HttpClient>().GetHttpClient()),
        pgCluster_(context.FindComponent<components::Postgres>("database").GetCluster()),
        updatedbTask()
    {
        updatedbTask.Start("aboba", utils::PeriodicTask::Settings(updateInterval), [this](){
            // LOG_ERROR() << "TASK RUN";
            task();
        });
        using storages::postgres::ClusterHostType;

        constexpr auto createUniversitiesTableQ = R"~(
            CREATE TABLE IF NOT EXISTS universities (
                name TEXT,
                link TEXT,
                PRIMARY KEY (name)
            )
        )~";
        pgCluster_->Execute(ClusterHostType::kMaster, createUniversitiesTableQ);

        constexpr auto addUniversityesQ = R"~(
            INSERT INTO universities (name) VALUES
                ('Lomonosov Moscow State University'),
                ('Moscow Institute of Physics & Technology'),
                ('National Research Nuclear University MEPhI (Moscow Engineering Physics Institute'),
                ('HSE University (National Research University Higher School of Economics'),
                ('Novosibirsk State University'),
                ('Tomsk State University'),
                ('Sechenov First Moscow State Medical University'),
                ('Saint Petersburg State University'),
                ('Peter the Great St. Petersburg Polytechnic University'),
                ('ITMO University'),
                ('Skolkovo Institute of Science & Technology'),
                ('Kazan Federal University'),
                ('Tomsk Polytechnic University'),
                ('South Ural State University'),
                ('Peoples Friendship University of Russia'),
                ('Ural Federal University'),
                ('National University of Science & Technology (MISIS)'),
                ('Lobachevsky State University of Nizhni Novgorod'),
                ('Saratov State University'),
                ('Far Eastern Federal University'),
                ('Siberian Federal University'),
                ('Southern Federal University'),
                ('Pirogov Russian National Research Medical University'),
                ('Bauman Moscow State Technical University'),
                ('Mendeleev University of Chemical Technology of Russia'),
                ('MIREA - Russian Technological University'),
                ('Belgorod State University'),
                ('Novosibirsk State Technical University'),
                ('Samara National Research University'),
                ('Tomsk State Pedagogical University'),
                ('Tomsk State University of Control Systems & Radioelectronics')
            ON CONFLICT DO NOTHING
            )~";
        pgCluster_->Execute(ClusterHostType::kMaster, addUniversityesQ);

        constexpr auto createAuthorsTableQ = R"~(
            CREATE TABLE IF NOT EXISTS authors (
                name TEXT,
                link TEXT,
                affilation TEXT,
                PRIMARY KEY (name),
                FOREIGN KEY (affilation) REFERENCES universities (name)
            )
        )~";
        pgCluster_->Execute(ClusterHostType::kMaster, createAuthorsTableQ);

        constexpr auto createArticlesTableQ = R"~(
            CREATE TABLE IF NOT EXISTS articles (
                title TEXT NOT NULL,
                author TEXT NOT NULL,
                journal TEXT NOT NULL,
                year SMALLINT NOT NULL,
                PRIMARY KEY(title, author),
                FOREIGN KEY (author) REFERENCES authors (name)
            )
        )~";
        pgCluster_->Execute(ClusterHostType::kMaster, createArticlesTableQ);

        constexpr auto createVarTableQ = R"~(
            CREATE TABLE IF NOT EXISTS var (
                param_name TEXT,
                param_value TEXT NOT NULL,
                PRIMARY KEY (param_name)
            )
        )~";
        pgCluster_->Execute(ClusterHostType::kMaster, createVarTableQ);

        constexpr auto insertDefaultVarsQ = R"~(
            INSERT INTO var (param_name, param_value) VALUES
                ('last_update_hash', 'empty')
            ON CONFLICT DO NOTHING
        )~";
        pgCluster_->Execute(ClusterHostType::kMaster, insertDefaultVarsQ);
    }

    constexpr static size_t downloadPartSize = 100'000;
    constexpr static size_t outBufferSize = 5'000;
    constexpr static std::chrono::hours updateInterval{24};

protected:
    clients::http::Client& httpClient_;
    storages::postgres::ClusterPtr pgCluster_;
    utils::PeriodicTask updatedbTask;
};

} // namespace dblpservice


template <>
inline constexpr bool
    USERVER_NAMESPACE::components::kHasValidate<dblpservice::UpdateDB> = true;

int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList()
        .Append<clients::dns::Component>()
        .Append<components::Postgres>("database")
        .Append<dblpservice::UpdateDB>()
        .Append<components::TestsuiteSupport>()
        .Append<userver::server::handlers::Ping>()
        .Append<userver::components::HttpClient>();
    return utils::DaemonMain(argc, argv, component_list);
}
