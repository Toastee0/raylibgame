# Simulation Design Goals and Strategy

## effecient coding
-this app should be performant on modern equipment and should take advantange of the multi-core cpu
-this app should add support for offloading a portion of the sim workload to cuda. 

## Ecosystem Overview
- Closed ecosystem with weather, rock weathering, water, soil/sand, and plant life.
- All cycles (water, nutrient, erosion) should be self-sustaining and balanced for long-term simulation.

## Water System
- Water can flow, evaporate, condense, and participate in the moisture cycle.
- Infinite water input and infinite drain points allow for river flow through the simulation.
- Water can erode soil and rock, carrying sand particles.
- Water deposits sand at the bottom; enough sand in water can form new soil.
- Water cells can hold sub-particles: moisture, sand, nutrient.
- Air cells can also hold moisture, sand, and nutrient.

## Rock and Weathering
- Rock is weathered very slowly by water (e.g., 1 million sand particles = 1 rock cell).
- Weathering creates sand, which is a sub-particle for soil.

## Soil and Sand
- Soil is composed of sand + nutrient + water.
- Soil can hold and transfer water, but can also become dry.
- Dry soil releases small amounts of sand into adjacent air.
- Soil can be eroded by water.

## Nutrient Cycle
- Nutrient is created by plants (especially when they die).
- Moss is a special plant that grows on wet rock or sand and releases nutrient when it dies.
- Second-tier plants require soil with nutrient to grow.
- Plant growth and death add nutrient to the soil.

## Plant System
- Moss: grows on wet rock/sand, releases nutrient on death.
- Other plants: require nutrient-rich soil, contribute to nutrient cycle.

## Moisture and Weather System
- Moisture cycle: water evaporates or condenses based on dew point and pressure.
- Moisture in air can condense into raindrops and fall.
- Clouds form under the top border or under surfaces that block upward movement of moist air.
- Cool surfaces cause moisture to condense out as appropriate.
- Rain/cloud system should be balanced: not too much, not too little, but enough to be interesting.

## Erosion and Deposition
- Water erodes soil and rock, carries sand, deposits sand at the bottom.
- Water with enough sand can form new soil; excess water seeks space in the connected water blob.

## Simulation Balance
- All cycles should be tuned for stability and interest.
- Rain/cloud formation should not overwhelm the system but should be visually and mechanically engaging.

---

*This file is a living document. Update as simulation features and mechanics evolve.*
