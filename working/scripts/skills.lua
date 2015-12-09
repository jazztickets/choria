Skill_Attack = {}

function Skill_Attack.ResolveBattleUse(Source, Target, ActionResult)
	Damage = math.max(Source.GenerateDamage() - Target.GenerateDefense(), 0)

	ActionResult.DamageDealt = Damage
	ActionResult.TargetHealthChange = -Damage

	return ActionResult
end
