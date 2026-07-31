#include "CalculateAllocAlignmentPostProcessor.h"

#include "Domain/Definition/ArrayDeclarationModifier.h"
#include "Domain/Evaluation/OperandStatic.h"

#include <algorithm>
#include <unordered_map>

namespace
{
    class AllocAlignmentCalculator
    {
    public:
        explicit AllocAlignmentCalculator(const unsigned pointerAlignment)
            : m_pointer_alignment(pointerAlignment)
        {
        }

        unsigned AlignmentOf(const DataDefinition* definition)
        {
            if (const auto existing = m_cache.find(definition); existing != m_cache.end())
                return existing->second;

            m_cache.emplace(definition, 0u);

            const auto result = Calculate(definition);
            m_cache[definition] = result;

            return result;
        }

        unsigned AlignmentOfDeclaration(const TypeDeclaration* declaration)
        {
            if (declaration->GetForceAlignment())
                return declaration->GetAlignment();

            for (const auto& modifier : declaration->m_declaration_modifiers)
            {
                if (modifier->GetType() == DeclarationModifierType::POINTER)
                    return m_pointer_alignment;
            }

            return AlignmentOf(declaration->m_type);
        }

    private:
        unsigned Calculate(const DataDefinition* definition)
        {
            if (definition->GetForceAlignment())
                return definition->GetAlignment();

            const auto* withMembers = dynamic_cast<const DefinitionWithMembers*>(definition);
            if (!withMembers)
                return definition->GetAlignment();

            auto alignment = 0u;
            for (const auto& member : withMembers->m_members)
                alignment = std::max(alignment, AlignmentOfDeclaration(member->m_type_declaration.get()));

            return alignment;
        }

        unsigned m_pointer_alignment;
        std::unordered_map<const DataDefinition*, unsigned> m_cache;
    };

    void ApplyTo(AllocAlignmentCalculator& calculator, DefinitionWithMembers& definition)
    {
        definition.m_alloc_alignment = calculator.AlignmentOf(&definition);

        for (const auto& member : definition.m_members)
        {
            auto& declaration = *member->m_type_declaration;
            declaration.m_alloc_alignment = declaration.GetForceAlignment() ? declaration.GetAlignment() : calculator.AlignmentOfDeclaration(&declaration);
        }
    }
} // namespace

bool CalculateAllocAlignmentPostProcessor::PostProcess(IDataRepository* repository)
{
    const auto allocWordSize = repository->GetAllocAlignmentWordSize();
    if (allocWordSize == repository->GetWordSize())
        return true;

    AllocAlignmentCalculator calculator(GetPointerSizeForWordSize(allocWordSize));

    for (auto* structDefinition : repository->GetAllStructs())
        ApplyTo(calculator, *structDefinition);

    for (auto* unionDefinition : repository->GetAllUnions())
        ApplyTo(calculator, *unionDefinition);

    return true;
}
