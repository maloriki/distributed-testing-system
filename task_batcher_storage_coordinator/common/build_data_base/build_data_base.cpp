#include "build_data_base.h"

namespace NDTS::NTabasco {

std::string GetAuthenticationString(const TBuildDataBaseConfig& config) {
    std::string authenticationString;

    authenticationString
        .append("dbname = ").append(config.dbname()).append(" ")
        .append("user = ").append(config.user()).append(" ")
        .append("password = ").append(config.password()).append(" ")
        .append("hostaddr = ").append(config.hostaddr()).append(" ")
        .append("port = ").append(config.port()).append(" ");

    return authenticationString;
}

TBuildDataBase::TBuildDataBase(const TBuildDataBaseConfig& config)
    : connection_(GetAuthenticationString(config))
{
    if (!connection_.is_open()) {
        throw std::runtime_error("db connection error");
    }
}

void TBuildDataBase::UploadInitScript(std::string scriptName, std::string content) {
    pqxx::nontransaction nonTx(connection_);
    nonTx.exec_params("INSERT INTO initScripts (scriptName, content) VALUES ($1, $2)", scriptName, content);
}

void TBuildDataBase::UploadExecuteScript(std::string scriptName, std::string content) {
    pqxx::nontransaction nonTx(connection_);
    nonTx.exec_params("INSERT INTO executeScripts (scriptName, content) VALUES ($1, $2)", scriptName, content);
}

void TBuildDataBase::CreateBuild(uint64_t executeScriptId, uint64_t initScriptId) {
    pqxx::nontransaction nonTx(connection_);
    nonTx.exec_params("INSERT INTO builds (scriptName, content) VALUES ($1, $2)", executeScriptId, initScriptId);
}

TScripts TBuildDataBase::GetScripts(uint64_t buildId) {
    pqxx::nontransaction nonTx(connection_);
    pqxx::result scriptsId = nonTx.exec_params("SELECT * FROM builds WHERE id = $1", buildId);

    uint64_t initScriptId = scriptsId[0][1].as<uint64_t>();
    pqxx::result initScriptRows = nonTx.exec_params("SELECT * FROM initScripts WHERE id = $1", initScriptId);

    uint64_t executeScriptId = scriptsId[0][2].as<uint64_t>();
    pqxx::result executeScriptRows = nonTx.exec_params("SELECT * FROM executeScripts WHERE id = $1", executeScriptId);

    return TScripts{
        .initScript = initScriptRows[0][1].as<std::string>(),
        .executeScript = executeScriptRows[0][1].as<std::string>()
    };  
}

} // end of NDTS::TTabasco namespace
