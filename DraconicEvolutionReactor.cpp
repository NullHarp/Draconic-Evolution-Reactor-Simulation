// DraconicEvolutionReactor.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Reactor.h"
#include <chrono>
#include <cstdio>
#include "Reactor.cpp"

using namespace std;


int main()
{
	std::ios::sync_with_stdio(false);
	Reactor reactor;
    printf("Starting Simulation!\n");
	double fuelBlocks = 8;
	double fuelIngots = fuelBlocks * 9;
	double fuelNuggets = fuelIngots * 9;
	long perciseInput = 0;
	int input = 0;
	int mode = 0;

	printf("Input Operational Mode:\nDefault: 0, Custom Fuel: 1, Full Custom: 2\n");
	scanf_s("%d",&input);
	if (input == 0)
	{
		mode == input;
		printf("Using Default parmaters.\n");
	}
	else if (input == 1)
	{
		mode == input;
		printf("Using Custom Fuel parmaters.\n");
		printf("Enter fuel in blocks:\n");
		scanf_s("%d", &input);
		if (input > 0 && input <= 8)
		{
			fuelBlocks = input;
			fuelIngots = fuelBlocks * 9;
			fuelNuggets = fuelIngots * 9;
		}
		printf("Enter fuel in ingots:\n");
		scanf_s("%d", &input);
		if (input > 0 && input <= 8)
		{
			fuelIngots = (fuelBlocks * 9) + input;
			fuelNuggets = fuelIngots * 9;
		}
		printf("Enter fuel in nuggets:\n");
		scanf_s("%d", &input);
		if (input > 0 && input <= 8)
		{
			fuelNuggets = (fuelIngots * 9) + input;
		}
	}
	else if (input == 2)
	{
		// Don't use this yet, its not finished and currently makes the reactor die.
		mode == input;
		printf("Using Full Custom parmaters.\n");
		printf("Input Temperature:\n");
		scanf_s("%ld", &perciseInput);
		reactor.temperature = perciseInput;
		printf("\nInput Field Strength:\n");
		scanf_s("%ld", &perciseInput);
		reactor.shieldCharge = perciseInput;
		printf("\nInput Energy Saturation:\n");
		scanf_s("%ld", &perciseInput);
		reactor.saturation = perciseInput;
		printf("\nInput Fuel:\n");
		scanf_s("%ld", &perciseInput);
		reactor.reactableFuel = perciseInput;
		printf("\nInput Chaos:\n");
		scanf_s("%ld", &perciseInput);
		reactor.convertedFuel = perciseInput;
		reactor.reactorState = Reactor::ReactorState::RUNNING;

	}
	reactor.reactableFuel = (double)fuelNuggets * 16;

	// You dont have to keep anything past here, this is just an example of what a program may look like
	reactor.attemptInitialization();
	reactor.chargeReactor();
	int tick = 0;
	int initial_take = 2000000;
	int initial_give = 600000;
	int current_take_rate = 1000;
	int current_give_rate = 1030;
	int take = initial_take;
	int give = initial_give;
	
	double power_peak = 0;
	double last_power = 0;

	int failed = 0;
	int successful = 0;

	auto startTime = std::chrono::high_resolution_clock::now();
	while (true)
	{
		if (reactor.canActivate())
		{
			reactor.activateReactor();
		}
		else
		{
			reactor.injectEnergy((long)3000000);
		}
		if (reactor.reactorState == Reactor::ReactorState::RUNNING)
		{
			if ((reactor.saturation - take) >= 0)
			{
				reactor.saturation = reactor.saturation - take;
			}
			if (reactor.shieldCharge > (double)18000000)
			{
				take = take + current_take_rate;
			}
			else
			if (reactor.shieldCharge < (double)18000000)
			{
				give = give + current_give_rate;
			}
			reactor.injectEnergy(give);
		}

		if (reactor.temperature > 10000)
		{
			take = take - 1500;
			give = give - 1000;
		}

		last_power = reactor.generationRate;
		reactor.updateCoreLogic();
		if (reactor.generationRate > last_power && reactor.generationRate > power_peak)
		{
			power_peak = reactor.generationRate;
		}

		if (reactor.reactableFuel < 500)
		{
			reactor.shutdownReactor();
		}
		if (reactor.saturation > 250000000)
		{
			take = take + 1;
		}
		if (reactor.reactorState == Reactor::ReactorState::BEYOND_HOPE || reactor.reactorState == Reactor::ReactorState::STOPPING)
		{
			printf("NullHarp (C)2023 | Draconic Evolution Reactor Simulation\nTemp:    %f\nShield:  %f%\nSat:     %ld%\nConFuel: %f\nFuel:    %f\nGenRate: %d\nT: %d\nH: %d\nM: %d\nS: %d\n\nTake: %d Give: %d\nPeak Power: %fM\n%d iterations, %d fails, %d successes.\n\n", 
				reactor.temperature, (reactor.shieldCharge / reactor.maxShieldCharge) * 100, 
				reactor.saturation, reactor.convertedFuel, reactor.reactableFuel, (int)reactor.generationRate,
				tick, ((tick / 20) / 60) / 60, ((tick / 20) / 60) % 60, (tick / 20) % 60, current_take_rate, current_give_rate, (long)power_peak / 1e+6, (failed + successful), failed, successful);
		}
		if (reactor.reactorState == Reactor::ReactorState::BEYOND_HOPE)
		{
			failed++;
			auto endTime = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
			printf("Peak Power: %fM\nTotal IRL Time: %f\nSimulation FAILED, Reactor Meltdown initated\n", (long)power_peak / 1e+6, duration.count() / 1e+6);
			printf("Initiating new paramaters: Take: %d Give: %d\n", current_take_rate, current_give_rate);
			//printf("%d iterations, %d fails, %d successes.\n", (failed + successful), failed, successful);
			take = initial_take;
			give = initial_give;
			tick = 0;
			current_give_rate = current_give_rate + 1;
			reactor.reactorState = Reactor::ReactorState::COLD;
			reactor.convertedFuel = 0;
			reactor.reactableFuel = (double)fuelNuggets * 16;
			reactor.chargeReactor();
			reactor.activateReactor();
			//exit(23);
			power_peak = 0;
			last_power = 0;
		}
		if (reactor.reactorState == Reactor::ReactorState::STOPPING)
		{
			successful++;
			auto endTime = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
			printf("Peak Power: %fM\nTotal IRL Time: %f\nSimulation HALTED, Reactor Shutdown initated\n", (long)power_peak / 1e+6, duration.count() / 1e+6);
			printf("Initiating new paramaters: Take: %d Give: %d\n", current_take_rate, current_give_rate);
			//printf("%d iterations, %d fails, %d successes.\n",(failed +successful),failed,successful);
			take = initial_take;
			give = initial_give;
			tick = 0;

			reactor.convertedFuel = 0;
			reactor.reactableFuel = (double)fuelNuggets * 16;
			reactor.activateReactor();
			//exit(24);
		}
		tick++;
	}
}