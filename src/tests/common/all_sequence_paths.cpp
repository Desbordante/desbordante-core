#include "tests/common/all_sequence_paths.h"

namespace tests {

namespace {
static auto const kSequenceDataDir =
        std::filesystem::current_path() / "input_data" / "sequence_data";
}  // namespace

std::filesystem::path const kMaxFemBaselinePath = kSequenceDataDir / "maxfem_baseline.txt";
std::filesystem::path const kMaxFemWindowPath = kSequenceDataDir / "maxfem_window.txt";
std::filesystem::path const kMaxFemParallelPath = kSequenceDataDir / "maxfem_parallel.txt";
std::filesystem::path const kMaxFemPruningPath = kSequenceDataDir / "maxfem_pruning.txt";

}  // namespace tests
