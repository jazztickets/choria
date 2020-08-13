
-- Storage for battle instance data
Battles = {}

-- Calculate basic weapon damage vs target's armor
function Battle_ResolveDamage(Action, Level, Source, Target, Result)

	-- Check for hit
	if Random.GetInt(1, 100) <= Source.HitChance - Target.Evasion then

		-- Get damage
		Change = {}
		Change.Damage, Crit = Action:GenerateDamage(Level, Source)
		if Crit == true then
			Result.Target.Crit = true
		end

		-- Call OnHit methods for buffs
		StaminaChange = 0
		BuffSound = 0
		for i = 1, #Target.StatusEffects do
			Effect = Target.StatusEffects[i]
			if Effect.Buff.OnHit ~= nil then
				Effect.Buff:OnHit(Target, Effect, Change, Result)
				if Change.Stamina ~= nil then
					StaminaChange = StaminaChange + Change.Stamina
				end
				if Change.BuffSound ~= nil then
					BuffSound = Change.BuffSound
				end
			end
		end

		-- Update stamina
		if StaminaChange ~= 0 then
			Result.Target.Stamina = StaminaChange
		end

		-- Set sound from buff
		if BuffSound ~= 0 then
			Result.Target.BuffSound = BuffSound
		end

		-- Apply damage block
		Change.Damage = math.max(Change.Damage - Target.DamageBlock, 0)

		-- Apply resistance
		Result.Target.DamageType = Action:GetDamageType(Source)
		Change.Damage = math.floor(Change.Damage * Target.GetDamageReduction(Action:GetDamageType(Source)) + Action:GetPierce(Source))

		-- Handle mana damage reduction
		Update = ResolveEnergyField(Target, Change.Damage)
		Result.Target.Health = Update.Health
		Result.Target.Mana = Update.Mana

		Hit = true
	else
		Result.Target.Miss = true
		Hit = false
	end

	return Hit
end

-- Convert damage into health/mana reduction based on target's mana reduction ratio
function ResolveEnergyField(Target, Damage)
	Update = {}
	Update.Health = 0
	Update.Mana = 0
	if Target.EnergyField > 0 then
		Update.Health = -Damage * (1.0 - Target.EnergyField * 0.01)
		Update.Mana = -Damage * Target.EnergyField * 0.01

		-- Remove fractions from damage
		Fraction = math.abs(Update.Health) - math.floor(math.abs(Update.Health))
		Update.Health = Update.Health + Fraction
		Update.Mana = Update.Mana - Fraction

		-- Carry over extra mana damage to health
		ResultingMana = Target.Mana + Update.Mana
		if ResultingMana < 0 then
			Update.Health = Update.Health + ResultingMana
			Update.Mana = Update.Mana - ResultingMana
		end
	else
		Update.Health = -Damage
	end

	return Update
end

-- Roll for proccing weapons
function WeaponProc(Source, Target, Result, IsSpell)
	if Target == nil then
		return
	end

	-- Check for main weapon
	Weapon = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND1)
	if Weapon == nil or Weapon.Proc == nil or (IsSpell == true and Weapon.SpellProc == 0) then
		return
	end

	-- Proc main weapon
	if (Weapon.SpellProc == 2) or (Weapon.Proc.SpellOnly == true and IsSpell == true) or (Weapon.Proc.SpellOnly == false and IsSpell == false) then
		Weapon.Proc:Proc(Random.GetInt(1, 100), Weapon, Source, Target, Result)
	end

	-- Check for off-hand
	WeaponOffHand = Source.GetInventoryItem(BAG_EQUIPMENT, INVENTORY_HAND2)
	if WeaponOffHand == nil or WeaponOffHand.Proc == nil or (IsSpell == true and WeaponOffHand.SpellProc == 0) then
		return
	end

	-- Proc off-hand weapon
	if (WeaponOffHand.SpellProc == 2) or (WeaponOffHand.Proc.SpellOnly == true and IsSpell == true) or (WeaponOffHand.Proc.SpellOnly == false and IsSpell == false) then
		WeaponOffHand.Proc:Proc(Random.GetInt(1, 100), WeaponOffHand, Source, Target, Result)
	end
end
