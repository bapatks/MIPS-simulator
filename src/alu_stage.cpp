#include "utils.h"
#include "mips_arch.h"

// =====================================================================================================================
AluStage::AluStage(bool allowLoadStoreOnly)
    :
    m_praluQ(2),
    m_isLoadStoreOnlyAlu(allowLoadStoreOnly)
{

}

// =====================================================================================================================
// Simulate ALU Stage
void AluStage::SimulateStage(
    ControlUnit* pControlUnit)
{
    if (m_isLoadStoreOnlyAlu)
    {
        SimulateLoadStoreOnlyStage(pControlUnit);
    }
    else
    {
        SimulateDataAluStage(pControlUnit);
    }
}

// =====================================================================================================================
// Simulate ALU Stage. This stage handles the calculation of address for memory (LW and SW) instructions only.
void AluStage::SimulateLoadStoreOnlyStage(
    ControlUnit* pControlUnit)
{
}

// =====================================================================================================================
// Simulate ALU Stage. This stage handles the calculation of all non-memory instructions.
void AluStage::SimulateDataAluStage(
    ControlUnit* pControlUnit)
{
}