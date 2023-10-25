#include <random>
#include <cstdio>
#include "Reactor.h"

using namespace rct;
using namespace std;

    //region ################# Core Logic ##################

    void Reactor::updateCoreLogic() {
        double sat;
        switch (reactorState) {
        case ReactorState::INVALID:
            updateOfflineState();
            break;
        case ReactorState::COLD:
            updateOfflineState();
            break;
        case ReactorState::WARMING_UP:
            initializeStartup();
            break;
        case ReactorState::RUNNING:
            updateOnlineState();

            if (failSafeMode && temperature < 2500 && (double)saturation / (double)maxSaturation >= 0.99) {
                printf("Reactor: Initiating Fail-Safe Shutdown!");
                shutdownReactor();
            }

            sat = (double)1 - (saturation / (double)maxSaturation);
            break;
        case ReactorState::STOPPING:
            updateOnlineState();
            if (temperature <= 2000) {
                reactorState = ReactorState::COOLING;
            }
            break;
        case ReactorState::COOLING:
            updateOfflineState();
            if (temperature <= 100) {
                reactorState = ReactorState::COLD;
            }
            break;
        }
    }

    /**
     * Update the reactors offline state.
     * This is responsible for things like returning the core temperature to minimum and draining remaining charge after the reactor shuts down.
     */

    void Reactor::updateOfflineState() {
        std::random_device rd;  // Seed the random number generator
        std::mt19937 gen(rd()); // Mersenne Twister engine
        std::uniform_real_distribution<double> distribution(0.0, 1.0);
        if (temperature > 20) {
            temperature = temperature - 0.5;
        }
        if (shieldCharge > 0) {
            shieldCharge = shieldCharge - (maxShieldCharge * 0.0005 * distribution(gen));
        }
        else if (shieldCharge < 0) {
            shieldCharge = 0;
        }
        if (saturation > 0) {
            saturation = shieldCharge - ((int)(maxSaturation * 0.000002 * distribution(gen)));
        }
        else if (saturation < 0) {
            saturation = 0;
        }
    }

    /**
     * This method is fired when the reactor enters the warm up state.
     * The first time this method is fired if initializes all of the reactors key fields.
     */
    void Reactor::initializeStartup() {
        if (!startupInitialized) {
            double totalFuel = reactableFuel + convertedFuel;
            maxShieldCharge = (totalFuel * 96.45061728395062 * 100);
            maxSaturation = ((int)(totalFuel * 96.45061728395062 * 1000));

            if (saturation > maxSaturation) {
                saturation = maxSaturation;
            }

            if (shieldCharge > maxShieldCharge) {
                shieldCharge = maxShieldCharge;
            }

            startupInitialized = true;
        }
    }

    void Reactor::updateOnlineState() {
        double coreSat = (double)saturation / (double)maxSaturation;         //1 = Max Saturation
        double negCSat = ((double)1 - coreSat) * (double)99;                                             //99 = Min Saturation. I believe this tops out at 99 because at 100 things would overflow and break.
        double temp50 = min((temperature / MAX_TEMPERATURE) * 50, (double)99);          //50 = Max Temp. Why? TBD
        double tFuel = convertedFuel + reactableFuel;                          //Total Fuel.
        double convLVL = ((convertedFuel / tFuel) * (double)1.3) - (double)0.3;                    //Conversion Level sets how much the current conversion level boosts power gen. Range: -0.3 to 1.0

        //region ============= Temperature Calculation =============

        double tempOffset = 444.7;    //Adjusts where the temp falls to at 100% saturation

        //The exponential temperature rise which increases as the core saturation goes down
        double tempRiseExpo = (negCSat * negCSat * negCSat) / (100 - negCSat) + tempOffset; //This is just terrible... I cant believe i wrote this stuff...

        //This is used to add resistance as the temp rises because the hotter something gets the more energy it takes to get it hotter
        double tempRiseResist = (temp50 * temp50 * temp50 * temp50) / (100 - temp50);       //^ Mostly Correct... The hotter an object gets the faster it dissipates heat into its surroundings to the more energy it takes to compensate for that energy loss.

        //This puts all the numbers together and gets the value to raise or lower the temp by this tick. This is dealing with very big numbers so the result is divided by 10000
        double riseAmount = (tempRiseExpo - (tempRiseResist * ((double)1 - convLVL)) + convLVL * 1000) / 10000;

        //Apply energy calculations.
        if (reactorState == ReactorState::STOPPING && convLVL < 1) {
            if (temperature <= 2001) {
                reactorState = ReactorState::COOLING;
                startupInitialized = false;
                return;
            }
            if (saturation >= maxSaturation * (double)0.99 && reactableFuel > (double)0) {
                temperature = temperature - (double)1 - convLVL;
            }
            else {
                temperature = temperature + riseAmount * 10;
            }
        }
        else {
            temperature = temperature + riseAmount * 10;
        }

        //        temperature.value = 18000;

                //endregion ================================================

                //region ============= Energy Calculation =============

        int baseMaxRFt = (int)((maxSaturation / (double)1000) * 1 * (double)1.5 * 10);
        int maxRFt = (int)(baseMaxRFt * ((double)1 + (convLVL * 2)));
        generationRate = ((double)1 - coreSat) * maxRFt;
        saturation = saturation + (int)generationRate;

        //endregion ===========================================b 

        //region ============= Shield Calculation =============

        tempDrainFactor = (temperature > 8000 ? 1 + ((temperature - 8000) * (temperature - 8000) * 0.0000025) : temperature > 2000 ? 1 : temperature > 1000 ? (temperature - 1000) / 1000 : 0);
        fieldDrain = (int)min(tempDrainFactor * max(0.01, ((double)1 - coreSat)) * (baseMaxRFt / 10.923556), (double)std::numeric_limits<int>::max()); //<(baseMaxRFt/make smaller to increase field power drain)

        double fieldNegPercent = (double)1 - (shieldCharge / maxShieldCharge);
        fieldInputRate = fieldDrain / fieldNegPercent;
        shieldCharge = shieldCharge - min((double)fieldDrain, shieldCharge);

        //endregion ===========================================

        //region ============== Fuel Calculation ==============

        fuelUseRate = tempDrainFactor * ((double)1 - coreSat) * (0.001 * 1 * 5); //<Last number is base fuel usage rate
        if (reactableFuel > 0) {
            convertedFuel = convertedFuel + fuelUseRate;
            reactableFuel = reactableFuel - fuelUseRate;
        }

        //endregion ===========================================

        //region Explosion
        if ((shieldCharge <= 0) && temperature > 2000 && reactorState != ReactorState::BEYOND_HOPE) {
            reactorState = ReactorState::BEYOND_HOPE;
        }
        //endregion ======
    }

    bool Reactor::canCharge() {
        if (reactorState == ReactorState::BEYOND_HOPE) {
            return false;
        }
        return (reactorState == ReactorState::COLD || reactorState == ReactorState::COOLING) && reactableFuel + convertedFuel >= 144;
    }

    bool Reactor::canActivate() {

        if (reactorState == ReactorState::BEYOND_HOPE) {
            return false;
        }

        return (reactorState == ReactorState::WARMING_UP || reactorState == ReactorState::STOPPING) && temperature >= 2000 && ((saturation >= maxSaturation / 2 && shieldCharge >= maxShieldCharge / 2) || reactorState == ReactorState::STOPPING);
    }

    bool Reactor::canStop() {
        if (reactorState == ReactorState::BEYOND_HOPE) {
            return false;
        }
        return reactorState == ReactorState::RUNNING || reactorState == ReactorState::WARMING_UP;
    }

    //region Notes for V2 Logic
    /*
     *
     * Calculation Order: WIP
     *
     * 1: Calculate conversion modifier
     *
     * 2: Saturation calculations
     *
     * 3: Temperature Calculations
     *
     * 4: Energy Calculations then recalculate saturation so the new value is reflected in the shield calculations
     *
     * 5: Shield Calculation
     *
     *
     */// endregion*/

     //endregion ############################################

     //region ############## User Interaction ###############
    void Reactor::chargeReactor() {
        if (canCharge()) {
            printf("Reactor: Start Charging\n");
            reactorState = ReactorState::WARMING_UP;
        }
    }

    void Reactor::activateReactor() {
        if (canActivate()) {
            printf("Reactor: Activate\n");
            reactorState = ReactorState::RUNNING;
        }
    }

    void Reactor::shutdownReactor() {
        if (canStop()) {
            printf("Reactor: Shutdown\n");
            reactorState = ReactorState::STOPPING;
        }
    }

    void Reactor::toggleFailSafe() {
        failSafeMode = !failSafeMode;
    }

    //region Initialization

    /**
     * Will will check if the structure is valid and if so will initialize the structure.
     */
    void Reactor::attemptInitialization() {
        printf("Reactor: Attempt Initialization\n");

        if (reactorState == ReactorState::INVALID) {
            if (temperature <= 100) {
                reactorState = ReactorState::COLD;
            }
            else {
                reactorState = ReactorState::COOLING;
            }
        }
        printf("Reactor: Structure Successfully Initialized!\n");
    }
    //endregion

    //endregion ############################################

    //region ################# Other Logic ##################

    long Reactor::injectEnergy(long energy) {
        long received = 0;
        if (reactorState == ReactorState::WARMING_UP) {
            if (!startupInitialized) {
                return 0;
            }
            if (shieldCharge < (maxShieldCharge / 2)) {
                received = min((int)energy, (int)(maxShieldCharge / 2) - (int)shieldCharge + 1);
                shieldCharge = shieldCharge + (double)received;
                if (shieldCharge > (maxShieldCharge / 2)) {
                    shieldCharge = maxShieldCharge / 2;
                }
            }
            else if (saturation < (maxSaturation / 2)) {
                received = min(energy, (maxSaturation / 2) - saturation);
                saturation = saturation + received;
            }
            else if (temperature < 2000) {
                received = energy;
                temperature = temperature + (double)received / ((double)1000 + (reactableFuel * 10));
                if (temperature > 2500) {
                    temperature = (double)2500;
                }
            }
        }
        else if (reactorState == ReactorState::RUNNING || reactorState == ReactorState::STOPPING) {
            double tempFactor = 1;

            //If the temperature goes past 15000 force the shield to drain by the time it hits 25000 regardless of input.
            if (temperature > 15000) {
                tempFactor = (double)1 - min((double)1, (temperature - (double)15000) / (double)10000);
            }

            shieldCharge = shieldCharge + min((energy * ((double)1 - (shieldCharge / maxShieldCharge))), maxShieldCharge - shieldCharge) * tempFactor;
            if (shieldCharge > maxShieldCharge) {
                shieldCharge = maxShieldCharge;
            }

            return energy;
        }
        return received;
    }
    void Reactor::removeEnergy(long energy)
    {
        if (reactorState == ReactorState::RUNNING || reactorState == ReactorState::STOPPING) {
            if (saturation - energy > 0)
            {
                saturation = saturation - energy;
            }
        }
    }

    //endregion #############################################