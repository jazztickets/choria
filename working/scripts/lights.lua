
-- Flicker --

Light_flicker = {}

function Light_flicker.Update(self, Light, Time)

	Light.a = Random.GetReal(0.3, 0.7)

	return Light
end
