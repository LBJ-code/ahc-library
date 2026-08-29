// AI事前作成コード公開元: https://github.com/LBJ-code/ahc-library

#pragma once

namespace ahc {

// score が小さいほどよいか、大きいほどよいか。
enum class Objective {
    minimize,
    maximize,
};

using OptimizationDirection = Objective;

}  // namespace ahc
