// atoms/infrastructure/batch_update_system.cpp
#include "batch_update_system.h"
#include "../atoms_template.h"
#include "../../vtk_viewer.h"
#include <spdlog/spdlog.h>

namespace atoms {
namespace infrastructure {

// ============================================================================
// BatchUpdateSystem 구현
// ============================================================================

BatchUpdateSystem::BatchUpdateSystem(AtomsTemplate* parent) 
    : parent(parent)
    , batchMode(false) {
    SPDLOG_DEBUG("BatchUpdateSystem initialized");
}

void BatchUpdateSystem::beginBatch() {
    if (batchMode) {
        SPDLOG_WARN("beginBatch() called while already in batch mode - ignoring");
        return;
    }
    
    batchMode = true;
    pendingAtomGroups.clear();
    pendingBondGroups.clear();
    batchStartTime = std::chrono::high_resolution_clock::now();
    
    SPDLOG_DEBUG("Batch mode started (atoms + bonds only)");
}

void BatchUpdateSystem::endBatch() {
    if (!batchMode) {
        SPDLOG_WARN("endBatch() called while not in batch mode - ignoring");
        return;
    }
    
    SPDLOG_DEBUG("Ending batch mode - processing {} atom groups, {} bond groups", 
                pendingAtomGroups.size(), pendingBondGroups.size());
    
    auto batchStart = std::chrono::high_resolution_clock::now();
    
    try {
        // 1. 원자 그룹 업데이트 (통합 시스템 - 전역 함수 호출)
        for (const std::string& symbol : pendingAtomGroups) {
            ::updateUnifiedAtomGroupVTK(symbol);  // 전역 함수 호출
            SPDLOG_DEBUG("Updated unified atom group: {}", symbol);
        }
        
        // 2. 결합 그룹 업데이트 (전역 함수 호출)
        for (const std::string& bondKey : pendingBondGroups) {
            if (parent) {
                parent->updateBondGroupVTK(bondKey);
            }
            SPDLOG_DEBUG("Updated bond group: {}", bondKey);
        }
        
        // 3. 배치 상태 리셋
        batchMode = false;
        pendingAtomGroups.clear();
        pendingBondGroups.clear();
        
        // 4. 단일 렌더링 호출
        VtkViewer::Instance().RequestRender();
        
        // 5. 성능 측정
        auto batchEnd = std::chrono::high_resolution_clock::now();
        float duration = std::chrono::duration<float, std::milli>(batchEnd - batchStart).count();
        
        SPDLOG_INFO("Batch update completed - {} atom groups, {} bond groups in {:.2f}ms",
                   pendingAtomGroups.size(), pendingBondGroups.size(), duration);
        
        updatePerformanceStats(duration);
        
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error during batch update: {}", e.what());
        forceBatchEnd();
        throw;
    }
}

void BatchUpdateSystem::forceBatchEnd() {
    if (batchMode) {
        SPDLOG_WARN("Force ending batch mode due to error or abnormal condition");
        batchMode = false;
        pendingAtomGroups.clear();
        pendingBondGroups.clear();
        
        // 안전한 상태로 복구하기 위해 렌더링 실행
        try {
            VtkViewer::Instance().RequestRender();
        } catch (const std::exception& e) {
            SPDLOG_ERROR("Failed to render during force batch end: {}", e.what());
        }
    }
}

void BatchUpdateSystem::scheduleAtomGroupUpdate(const std::string& symbol) {
    if (batchMode) {
        pendingAtomGroups.insert(symbol);
        SPDLOG_DEBUG("Scheduled unified atom group update: {}", symbol);
    } else {
        // 단일 아이템 배치를 생성해 일관된 경로로 처리
        BatchGuard guard(this);
        pendingAtomGroups.insert(symbol);
        SPDLOG_DEBUG("Scheduled unified atom group update (single-item batch): {}", symbol);
    }
}

void BatchUpdateSystem::scheduleBondGroupUpdate(const std::string& bondKey) {
    if (batchMode) {
        pendingBondGroups.insert(bondKey);
        SPDLOG_DEBUG("Scheduled bond group update: {}", bondKey);
    } else {
        // 단일 아이템 배치를 생성해 일관된 경로로 처리
        BatchGuard guard(this);
        pendingBondGroups.insert(bondKey);
        SPDLOG_DEBUG("Scheduled bond group update (single-item batch): {}", bondKey);
    }
}

void BatchUpdateSystem::updatePerformanceStats(float duration) {
    // AtomsTemplate의 updatePerformanceStats 호출
    // parent가 실제 통계 업데이트를 수행
    parent->updatePerformanceStatsInternal(duration, 
                                           pendingAtomGroups.size(), 
                                           pendingBondGroups.size());
}

// ============================================================================
// RAII BatchGuard 구현
// ============================================================================

BatchUpdateSystem::BatchGuard::BatchGuard(BatchUpdateSystem* sys) 
    : system(sys) {
    wasActivated = !sys->isBatchMode();
    if (wasActivated) {
        sys->beginBatch();
        SPDLOG_DEBUG("BatchGuard: Started new batch");
    } else {
        SPDLOG_DEBUG("BatchGuard: Using existing batch");
    }
}

BatchUpdateSystem::BatchGuard::~BatchGuard() {
    if (wasActivated && system) {
        try {
            system->endBatch();
            SPDLOG_DEBUG("BatchGuard: Ended batch successfully");
        } catch (const std::exception& e) {
            SPDLOG_ERROR("Exception in BatchGuard destructor: {}", e.what());
            try {
                system->forceBatchEnd();
            } catch (...) {
                SPDLOG_CRITICAL("Failed to force end batch in BatchGuard destructor");
            }
        }
    }
}

} // namespace infrastructure
} // namespace atoms
