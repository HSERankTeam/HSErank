#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <iostream>

#include "userver/engine/sleep.hpp"
#include "userver/logging/log.hpp"
#include "userver/storages/postgres/cluster_types.hpp"
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

#include <userver/engine/deadline.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include <zconf.h>
#include <zlib.h>

#include "xml_dblp.h"


namespace dblpservice {

class UpdateDB final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-updatedb";
    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequestThrow(
            const server::http::HttpRequest& request,
            server::request::RequestContext&) const override {

            const auto response = httpClient_.CreateNotSignedRequest()
                -> get("http://185.188.183.91/dblp.xml.gz.md5")
                -> timeout(std::chrono::seconds(15))
                -> retry()
                -> perform();
            if (!response->IsOk()) {
                LOG_ERROR() << response->body() << "dblp return status: " << response->status_code();
            }

            std::string_view availableDataHash = response->body_view();
            // TODO check if need update
            bool newDataAvailable = true;

            if (newDataAvailable) {
                // get length of content
                const auto response = httpClient_.CreateRequest()
                    -> head("http://185.188.183.91/dblp.xml.gz")
                    -> timeout(std::chrono::seconds(15))
                    -> retry()
                    -> perform();
                std::string ans;
                ans += std::to_string(response->status_code());
                ans += "<br><br>";
                for (auto&[key, val] : response->headers()) {
                    ans += key;
                    ans += " = ";
                    ans += val;
                    ans += "<br>";
                }
                uint64_t length;
                try {
                    length = std::stoull(response->headers().at("Content-Length"));
                } catch (...) {
                    LOG_ERROR() << "invalid Content-Length: " << length;
                    throw;
                }

                // const auto r = httpClient_.CreateRequest()
                //     -> get("https://dblp.org/xml/dblp.xml.gz")
                //     -> timeout(std::chrono::seconds(15))
                //     -> retry()
                //     -> headers({{"Range", "bytes=0-10240"}})
                //     -> perform();

                z_stream stream = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
                int result = inflateInit2(&stream, MAX_WBITS + 16);
                if (result != Z_OK) {
                    LOG_ERROR() << "cannot initialize zstream";
                    throw std::runtime_error("");
                }
                // LOG_DEBUG() << "LENGTH " << length; 
                dblpXmlArticleParser parser({"www"});
                for (size_t i = 0; i < length; i += downloadPartSize) {
                    LOG_ERROR() << double(i) / length * 100 << " PERCENTS ";
                    LOG_DEBUG() << "LOOP STARTED";
                    auto from = std::to_string(i);
                    auto to = std::to_string(std::min(i + downloadPartSize - 1, length - 1));
                    const auto response = httpClient_.CreateRequest()
                    -> get("http://185.188.183.91/dblp.xml.gz")
                    -> timeout(std::chrono::seconds(15))
                    -> retry(10)
                    -> headers({{"Range", "bytes="+from+"-"+to}})
                    -> perform();

                    LOG_DEBUG() << "RECIVED " << response->status_code();

                    if (response->status_code() != 206 && response->status_code() != 200) {
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
                    // break;
                }
                if (inflateEnd(&stream) != Z_OK) {
                    throw std::runtime_error("failed inflateEnd");
                }

                return "UPDATED";
            }

        return "UPDATE NOT NEED";
    }

    void sendArticlesToDb(const std::vector<articlePart>& data) const {
        if (data.empty()) return;
        std::string request;
        request += "INSERT INTO articles VALUES ";
        for (const auto& article : data) {
            for (const auto& author : article.author) {
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
            }
        }
        request.pop_back();
        request += R"~(
            ON CONFLICT (title, author) DO NOTHING
            )~";
        LOG_DEBUG() << "ABOBAA" << request;
        using storages::postgres::ClusterHostType;
        auto result = pgCluster_->Execute(ClusterHostType::kMaster, request);
    }

    static void append(std::string& request, const std::string& data) {
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
        pgCluster_(context.FindComponent<components::Postgres>("database").GetCluster())
    {
        // TODO move to db construction component
        using storages::postgres::ClusterHostType;

        constexpr auto createTableQ = R"~(
            CREATE TABLE IF NOT EXISTS articles (
                title TEXT NOT NULL,
                author TEXT NOT NULL,
                journal TEXT NOT NULL,
                year SMALLINT NOT NULL,
                PRIMARY KEY(title, author)
            )
        )~";

        LOG_DEBUG() << createTableQ;
        
        pgCluster_->Execute(ClusterHostType::kMaster, createTableQ);
        
        LOG_DEBUG() << "TABLE CREATED OR EXISTS";
        // TODO create index
        // constexpr auto createAuthorIndexQ = R"~(
        //     CREATE INDEX IF NOT EXISTS 
        // )~";
        // pgCluster_->Execute(ClusterHostType::kMaster, createTableQ);
    }

    constexpr static size_t downloadPartSize = 100'000;
    constexpr static size_t outBufferSize = 100'000;

protected:
    clients::http::Client& httpClient_;
    storages::postgres::ClusterPtr pgCluster_;
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
