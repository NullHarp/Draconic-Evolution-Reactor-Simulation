#pragma once

namespace rct {

class Reactor {
    public:enum class ReactorState {
        INVALID,
        /**
         * The reactor is offline and cold.
         * In this state it is possible to add/remove fuel.
         */
         COLD,
         /**
          * Reactor is heating up in preparation for startup.
          */
          WARMING_UP,
          //AT_TEMP(true), Dont think i need this i can just have a "Can Start" check that checks the reactor is in the warm up state and temp is at minimum required startup temp.
          /**
           * Reactor is online.
           */
           RUNNING,
           /**
            * The reactor is shutting down..
            */
            STOPPING,
            /**
             * The reactor is offline but is still cooling down.
             */
             COOLING,
             BEYOND_HOPE
    };
    //region =========== Core Logic Fields ===========
    /**
     * This is the current operational state of the reactor.
     */
    public:enum class ReactorState reactorState = ReactorState::INVALID;
      /**
       * Remaining fuel that is yet to be consumed by the reaction.
       */
      public:double reactableFuel = 0;
      /**
       * Fuel that has been converted to chaos by the reaction.
       */
      public:double convertedFuel = 0;
      public:double temperature = 20;
      public:double MAX_TEMPERATURE = 10000;

      public:double shieldCharge = 0;
      public:double maxShieldCharge = 0;

      /**
       * This is how saturated the core is with energy.
       */
      public:long saturation = 0;
      public:long maxSaturation = 0;


      public:double tempDrainFactor = 0;
      public:double generationRate = 0;
      public:int fieldDrain = 0;
      public:double fieldInputRate = 0;
      public:double fuelUseRate = 0;

      public:bool startupInitialized = false;
      public:bool failSafeMode = false;

      //endregion ======================================

      //region ################# Core Logic ##################

      public:void updateCoreLogic();

      /**
       * Update the reactors offline state.
       * This is responsible for things like returning the core temperature to minimum and draining remaining charge after the reactor shuts down.
       */
      private:void updateOfflineState();

       /**
        * This method is fired when the reactor enters the warm up state.
        * The first time this method is fired if initializes all of the reactors key fields.
        */
      private:void initializeStartup();

      private:void updateOnlineState();

      public:bool canCharge();

      public:bool canActivate();

      public:bool canStop();

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

      public:void chargeReactor();

      public:void activateReactor();

      public:void shutdownReactor();

      public:void toggleFailSafe();

      //region Initialization

      /**
       * Will will check if the structure is valid and if so will initialize the structure.
       */
      public:void attemptInitialization();
      //endregion

      //endregion ############################################

      //region ################# Other Logic ##################

      public:long injectEnergy(long energy);

      public:void removeEnergy(long energy);
      //endregion #############################################
};
}