
-- Flicker --

Light_flicker = {}
Light_flicker.Values = {}
Light_flicker.Count = 100
Light_flicker.Speed = 15
Light_flicker.Min = 0.8
Light_flicker.Max = 1.0
for i = 1, Light_flicker.Count do
	Light_flicker.Values[i] = Random.GetReal(Light_flicker.Min, Light_flicker.Max)
end

function Light_flicker.Update(self, Light, Time)

	Index = (Light.Seed + math.floor(Time * self.Speed)) % self.Count
	Light.a = Light.a * self.Values[Index + 1]

	return Light
end
