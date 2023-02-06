#include "userver/clients/http/client.hpp"
#include "userver/logging/log.hpp"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <userver/server/handlers/ping.hpp>
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>

#include <zconf.h>
#include <zlib.h>

#include <iostream>

// #include <userver/crypto/>

// #include <userver/utils/>

namespace dblpservice {

class UpdateDB final : public server::handlers::HttpHandlerBase {
public:
    // `kName` is used as the component name in static config
    static constexpr std::string_view kName = "handler-updatedb";

    // Component is valid after construction and is able to accept requests
    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequestThrow(
            const server::http::HttpRequest& request,
            server::request::RequestContext&) const override {

            // auto client_ =

            const auto response = httpClient_.CreateNotSignedRequest()
                -> get("https://dblp.org/xml/dblp.xml.gz.md5")
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
                    -> head("https://dblp.org/xml/dblp.xml.gz")
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
                ans += "<br>";
                ans += "Len = ";
                ans += std::to_string(length);
                ans += "<br><br>";

                const auto r = httpClient_.CreateRequest()
                    -> get("https://dblp.org/xml/dblp.xml.gz")
                    -> timeout(std::chrono::seconds(15))
                    -> retry()
                    -> headers({{"Range", "bytes=0-10240"}})
                    -> perform();

                ans += "<br> PART: <br>";
                if (r->status_code() != 206) {
                    LOG_ERROR() << "partial download " << r->status_code();
                }
                ans += r->body();

                ans += "<br><br>";

                // decompress
                {
                    uint8_t inbuff[10241];
                    uint8_t outbuff[10241];

                    LOG_ERROR() << (int)r->body()[0] <<  ' ' << (int)r->body()[1] << ' ' << (int)r->body()[2];

                    memcpy(inbuff, r->body().c_str(), 1500);

                    LOG_ERROR() << inbuff[0] << ' ' << inbuff[1] << ' '  << inbuff[2];

                    z_stream stream = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

                    // stream.zalloc = &malloc;

                    int result = inflateInit2(&stream, MAX_WBITS + 16);
                    LOG_ERROR() << result << ' ' << (void*)stream.zalloc << ' ' << (void*)stream.zfree;
                    stream.avail_in = 1000;
                    stream.next_in = inbuff;
                    stream.avail_out = 10241;
                    stream.next_out = outbuff;
                    

                    int status = inflate(&stream, Z_NO_FLUSH);
                    inflateEnd(&stream);

                    uint32_t nbytes = 10241 - stream.avail_out;

                    std::string mem(outbuff, outbuff + nbytes);
                    ans += mem;

                    

                    LOG_ERROR() << '\n' << nbytes << mem << ' ' << status << ' ' << stream.msg << '\n';
                    return mem;
                }

                return ans;
            }

        return response->body();
    }



    UpdateDB(const components::ComponentConfig& config,
            const components::ComponentContext& context):
        server::handlers::HttpHandlerBase(config, context),
        httpClient_(context.FindComponent<components::HttpClient>().GetHttpClient())
    {}

    // UpdateDB() : httpClient_(ClientSettings settings, engine::TaskProcessor &fs_task_processor)

protected:
    clients::http::Client& httpClient_;
};

}  // namespace samples::helloabc

// This forces userver to validate provided static config of Hello handler
// against the schema.
template <>
inline constexpr bool
    USERVER_NAMESPACE::components::kHasValidate<dblpservice::UpdateDB> = true;

int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList()
        .Append<dblpservice::UpdateDB>()
        .Append<userver::server::handlers::Ping>()
        .Append<userver::components::HttpClient>();
    return utils::DaemonMain(argc, argv, component_list);
}
