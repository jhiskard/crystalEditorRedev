// atoms/infrastructure/batch_update_system.h
#pragma once
#include <set>
#include <string>
#include <chrono>

class AtomsTemplate; // Forward declaration

namespace atoms {
namespace infrastructure {

/**
 * @brief 배치 업데이트 시스템 - 렌더링 최적화를 위한 업데이트 스케줄링
 * 
 * 다수의 원자/결합 업데이트를 모아서 한 번에 처리하여 
 * VTK 렌더링 호출을 최소화합니다.
 */
class BatchUpdateSystem {
public:
    explicit BatchUpdateSystem(AtomsTemplate* parent);
    
    // ========================================================================
    // 배치 모드 제어
    // ========================================================================
    
    /**
     * @brief 통합 배치 업데이트 시작
     * 
     * 이 시점부터 모든 원자/결합 그룹 업데이트를 지연시킵니다.
     * 중첩 호출은 무시됩니다.
     */
    void beginBatch();
    
    /**
     * @brief 통합 배치 업데이트 종료 및 실행
     * 
     * 지연된 모든 원자/결합 그룹 업데이트를 한 번에 실행하고
     * 단일 VTK 렌더링으로 성능을 최적화합니다.
     */
    void endBatch();
    
    /**
     * @brief 현재 배치 모드 상태 확인
     */
    bool isBatchMode() const { return batchMode; }
    
    /**
     * @brief 강제 배치 종료 (예외 상황용)
     */
    void forceBatchEnd();

    // ========================================================================
    // 통계 및 상태 조회
    // ========================================================================
    
    /**
     * @brief 대기 중인 원자 그룹 수 조회
     */
    size_t getPendingAtomGroupCount() const { return pendingAtomGroups.size(); }
    
    /**
     * @brief 대기 중인 결합 그룹 수 조회
     */
    size_t getPendingBondGroupCount() const { return pendingBondGroups.size(); }
    
    // ========================================================================
    // 업데이트 스케줄링
    // ========================================================================
    
    /**
     * @brief 원자 그룹 업데이트 스케줄링
     * 
     * 배치 모드에서는 지연 실행, 그렇지 않으면 즉시 실행
     */
    void scheduleAtomGroupUpdate(const std::string& symbol);
    
    /**
     * @brief 결합 그룹 업데이트 스케줄링
     * 
     * 배치 모드에서는 지연 실행, 그렇지 않으면 즉시 실행
     */
    void scheduleBondGroupUpdate(const std::string& bondKey);
    
    // ========================================================================
    // RAII BatchGuard 클래스
    // ========================================================================
    
    /**
     * @brief RAII 기반 자동 배치 관리자
     * 
     * 생성 시 beginBatch(), 소멸 시 endBatch()를 자동 호출합니다.
     * 예외 안전성을 보장하며 중첩 배치를 감지합니다.
     */
    class BatchGuard {
    private:
        BatchUpdateSystem* system;
        bool wasActivated;  // 중첩 배치 감지용
        
    public:
        explicit BatchGuard(BatchUpdateSystem* sys);
        ~BatchGuard();
        
        // 복사/이동 금지 (RAII 안전성)
        BatchGuard(const BatchGuard&) = delete;
        BatchGuard& operator=(const BatchGuard&) = delete;
        BatchGuard(BatchGuard&&) = delete;
        BatchGuard& operator=(BatchGuard&&) = delete;
    };
    
private:
    AtomsTemplate* parent;  // 부모 객체 참조 (콜백용)
    
    // 배치 상태
    bool batchMode;
    std::set<std::string> pendingAtomGroups;
    std::set<std::string> pendingBondGroups;
    
    // 성능 측정
    std::chrono::high_resolution_clock::time_point batchStartTime;
    
    /**
     * @brief 성능 통계 업데이트
     */
    void updatePerformanceStats(float duration);
};

} // namespace infrastructure
} // namespace atoms