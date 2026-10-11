#include "alife/genetics/lineage.hpp"

namespace alife::genetics {

void LineageLog::record(const LineageRecord& r) { records_.push_back(r); }

}  // namespace alife::genetics
