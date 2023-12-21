#include "testing_processor.h"
#include "tabasco_request_task.h"

#include <nlohmann/json.hpp>

#include <filesystem>

namespace NDTS::NTestingProcessor {

namespace fs = std::filesystem;

static const std::fs::path USER_ROOT_PATH = "/check";
static const std::fs::path INIT_SCRIPT_PATH = USER_ROOT_PATH / "init.sh";
static const std::fs::path EXECUTE_SCRIPT_PATH = USER_ROOT_PATH / "execute.sh";
static const std::fs::path USER_DATA_PATH = USER_ROOT_PATH / "userData";
static const std::fs::path USER_EXECUTABLE_PATH = USER_ROOT_PATH / "executable";
static const std::fs::path CHECKER_PATH = USER_ROOT_PATH / "checker";

TTestingProcessor::TTestingProcessor(const TTestingProcessorConfig& config)
    : container_(config.dockerContainerConfig())
{}

TTestingProcessor::Process(TTestingProcessorRequest request) {
    container_.Run();

    Prepare(request);
    Test(request);

    container_.Kill();
}

bool TTestingProcessor::Prepare(TTestingProcessorRequest& request, uint64* const batchCount) {
    TTabascoRequestTask tabascoRequestTask;

    TGetScriptsResponse response = tabascoRequestTask.GetScripts(request.taskId, request.buildId);

    container_.CreateFile(INIT_SCRIPT_PATH, std::move(scripts.initScript));
    container_.CreateFile(USER_DATA_PATH, std::move(request.userData));
    *batchCount = response.batchCount;

    int exitCode = container_.Exec(
        {INIT_SCRIPT_PATH, USER_DATA_PATH},
        std::nullopt,
        std::nullopt
    );
    
    if (exitCode != 0) {
        return false;
    }

    container_.CreateFile(EXECUTE_SCRIPT_PATH, std::move(scripts.executeScript));
    return true;
}

void TTestingProcessor::Test(TTestingProcessorRequest& request, uint64_t batchCount) {
    TTabascoRequestTask tabascoRequestTask;

    size_t testIndex = 1;

    for (size_t batchIndex = 0; batchIndex < batchCount; ++batchIndex) {
        auto tests = tabascoRequestTask.GetBatch(request.taskId, batchIndex);
        size_t testsSize = tests.inputTests.size();

        for (size_t i = 0; i < testsSize; ++i) {
            std::fs::path inputPath = std::to_string(testIndex) + "_test";
            std::fs::path outputPath = std::to_string(testIndex) + "_answer";

            container_.Exec(
                {EXECUTE_SCRIPT_PATH, USER_EXECUTABLE_PATH},
                inputPath,
                outputPath
            );

            std::string deserializedReport;
            nlohmann::json executeReport = nlohmann::json::parse(deserializedReport, nullptr, false);

            if (executeReport["exitCode"] != 0) {
                return;
            }

            if (executeReport["cpuTimeElapsedMicroSeconds"] > request.cpuTimeLimitSeconds * 1'000'000) {
                return;
            }

            if (executeReport["memorySpent"] > request.memoryLimit) {
                return; 
            }

            ++testIndex;
        }
    }
}

} // end of NDTS::NTestingProcessor namespace