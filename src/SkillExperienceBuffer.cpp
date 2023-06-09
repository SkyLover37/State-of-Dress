#include "SkillExperienceBuffer.h"


REL::Relocation<Actor_AdvanceSkill_Method> PlayerCharacter_AdvanceSkill;

float GetExperienceForLevel(RE::ActorValueInfo* info, std::uint32_t level)
{
	if (!info || !info->skill)
		return 0.0;

	float fSkillUseCurve = 0.f;
	auto settings = RE::GameSettingCollection::GetSingleton();
	if (!settings)
		return 0.0;

	auto skillUseCurve = settings->GetSetting("fSkillUseCurve");
	if (skillUseCurve)
		fSkillUseCurve = skillUseCurve->GetFloat();

	return info->skill->improveMult * powf(level, fSkillUseCurve) + info->skill->improveOffset;
}

#ifndef SKILLEXPERIENCEBUFFER_NOEXPERIMENTAL
struct SkillPointsByLevel
{
	std::uint16_t level = 0;
	float points = 0.0f;

	void update(RE::ActorValue skillId, std::uint32_t newLevelCap)
	{
		auto avi = RE::ActorValueList::GetSingleton()->GetActorValue(skillId);
		while (level < newLevelCap) {
			points += GetExperienceForLevel(avi, ++level);
		}
	}

} g_skillPointsByLevel[SkillCount];
#endif

float SkillExperienceBuffer::getExperience(RE::ActorValue skillId) const
{
    return expBuf[static_cast<std::uint32_t>(skillId)];
}

void SkillExperienceBuffer::addExperience(RE::ActorValue skillId, float points)
{
    expBuf[static_cast<std::uint32_t>(skillId) - static_cast<std::uint32_t>(FirstSkillId)] += points;
}

void SkillExperienceBuffer::flushExperience(float percent)
{
	for (RE::stl::enumeration<RE::ActorValue, std::uint32_t> skillId = FirstSkillId; skillId <= LastSkillId; ++skillId) {
		flushExperienceBySkill(skillId.get(), percent);
	}
}

void SkillExperienceBuffer::flushExperienceBySkill(RE::ActorValue skillId, float percent)
{
    logger::debug("flushing experience");
    int skillIndex = static_cast<std::uint32_t>(skillId) - static_cast<std::uint32_t>(FirstSkillId);
	auto pPC = RE::PlayerCharacter::GetSingleton();

#ifndef SKILLEXPERIENCEBUFFER_NOEXPERIMENTAL
	uint16_t skillLevel = pPC->GetBaseActorValue(skillId);

	if (skillStartLevel[skillIndex] == 0) {
		// Initialize skill start level.
		skillStartLevel[skillIndex] = skillLevel;
	}

	auto playerLevel = pPC->GetLevel();

	if (g_skillPointsByLevel[skillIndex].level < playerLevel) {
		if (g_skillPointsByLevel[skillIndex].level == 0) {
			g_skillPointsByLevel[skillIndex].level = skillLevel;
		}
		g_skillPointsByLevel[skillIndex].update(skillId, playerLevel);
	}
#endif

	float toAdd = expBuf[skillIndex] * percent;

	if (toAdd > 0.0f) {
        logger::debug("Experience to add: {0} {1} {2}", toAdd, skillId, percent);
#ifndef SKILLEXPERIENCEBUFFER_NOEXPERIMENTAL
        logger::debug("experimental");
		flushedExpBuf[skillIndex] += toAdd;
		if (flushedExpBuf[skillIndex] > g_skillPointsByLevel[skillIndex].points) {
			toAdd -= (flushedExpBuf[skillIndex] - g_skillPointsByLevel[skillIndex].points);
			flushedExpBuf[skillIndex] = g_skillPointsByLevel[skillIndex].points;
		}
#endif
        
        PlayerCharacter_AdvanceSkill(pPC, skillId, toAdd, 0, 0);
        
		logger::debug("Flush Exp of Skill : {}", skillId);
		expBuf[skillIndex] -= toAdd;
	}
    logger::debug("Finished flush");
}


void SkillExperienceBuffer::multExperience(float mult)
{
	for (RE::stl::enumeration<RE::ActorValue, std::uint32_t> skillId = RE::ActorValue::kOneHanded; skillId <= LastSkillId; ++skillId) {
		multExperienceBySkill(skillId.get(), mult);
	}
}

void SkillExperienceBuffer::multExperienceBySkill(RE::ActorValue skillId, float mult)
{
    expBuf[static_cast<std::uint32_t>(skillId) - static_cast<std::uint32_t>(FirstSkillId)] *= mult;
}

void SkillExperienceBuffer::clear()
{
	ZeroMemory(expBuf, sizeof(expBuf));
}

void SkillExperienceBuffer::clearBySkill(RE::ActorValue skillId)
{
    expBuf[static_cast<std::uint32_t>(skillId) - static_cast<std::uint32_t>(FirstSkillId)] = 0.0f;
}
