#include "userver/formats/json/value_builder.hpp"
#include "userver/storages/postgres/postgres_fwd.hpp"
#include <userver/server/handlers/ping.hpp>
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
 
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>
#include <utility>

// #include <userver/crypto/>

// #include <userver/utils/>
 
namespace api {
 
class getRank final : public server::handlers::HttpHandlerJsonBase {
public:
    // `kName` is used as the component name in static config
    static constexpr std::string_view kName = "handler-get-rank";
 
    getRank(const components::ComponentConfig& config,
            const components::ComponentContext& context) :
            server::handlers::HttpHandlerJsonBase(config, context),
            pgCluster_(context.FindComponent<components::Postgres>("database").GetCluster())
    {};

    formats::json::Value HandleRequestJsonThrow(
        const server::http::HttpRequest& request,
        const formats::json::Value& json,
        server::request::RequestContext&) const override {

        // TODO db request

        auto response = formats::json::ValueBuilder();
        for (int i = 0; i < 10; ++i) {
            response["universities"].PushBack("aboba");

            auto record = formats::json::ValueBuilder();
            record["name"] = "HSE";
            record["city"] = "Samara";
            record["metric1"] = 41;
            record["metric2"] = 10;
            record["metric3"] = 0.5;
        }
        return response.ExtractValue();
    }
    storages::postgres::ClusterPtr pgCluster_;
};
 
}  // namespace samples::hello
 
// This forces userver to validate provided static config of Hello handler
// against the schema.
template <>
inline constexpr bool
    USERVER_NAMESPACE::components::kHasValidate<api::getRank> = true;
 
int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList()
        .Append<components::Postgres>("database")
        .Append<api::getRank>()
        .Append<userver::server::handlers::Ping>();
    return utils::DaemonMain(argc, argv, component_list);
}
