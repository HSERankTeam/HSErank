#include <userver/server/handlers/ping.hpp>
#include <userver/utest/using_namespace_userver.hpp>
 
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>

// #include <userver/crypto/>

// #include <userver/utils/>
 
namespace abc {
 
class Hello final : public server::handlers::HttpHandlerBase {
public:
    // `kName` is used as the component name in static config
    static constexpr std::string_view kName = "get_";
 
    // Component is valid after construction and is able to accept requests
    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequestThrow(
            const server::http::HttpRequest& request,
            server::request::RequestContext&) const override {
            // auto json = 
    }
};
 
}  // namespace samples::hello
 
// This forces userver to validate provided static config of Hello handler
// against the schema.
template <>
inline constexpr bool
    USERVER_NAMESPACE::components::kHasValidate<abc::Hello> = true;
 
int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList()
        .Append<abc::Hello>()
        .Append<userver::server::handlers::Ping>();
    return utils::DaemonMain(argc, argv, component_list);
}