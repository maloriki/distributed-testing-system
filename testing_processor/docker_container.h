#pragma once

#include <filesystem>
#include <string>
#include <optional>
#include <vector>

#include "proto/docker_container.pb.h"

namespace NDTS::NTestingProcessor {

namespace fs = std::filesystem;

class TDockerContainer {
public:
    TDockerContainer(const TDockerContainerConfig& config);

    void Run();

    // return's exit code
    int Exec(
        std::vector<std::string> scriptArgs, 
        const std::optional<fs::path>& stdIn,
        const std::optional<fs::path>& stdOut
    );

    void MoveFileInside(const fs::path& outsidePath, const fs::path& insidePath);

    void CreateFile(const fs::path& path, std::string content);

    void Kill();

    void Remove();

    ~TDockerContainer();

private:
    std::string image_;
    uint32_t cpuCount_;
    std::string memoryLimit_;
    std::string memorySwapLimit_;
    uint32_t pidLimit_;
private:
    std::string containerId_;
};

} // end of NDTS::NTestingProcessor namespace
