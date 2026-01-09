/***************************************************
    Definition of most registers of the STPM3X Power
    Monitor chip. Fixed version based on register map.

    Benjamin Völker, voelkerb@me.com
    Embedded Systems
    University of Freiburg, Institute of Informatik
    
    Corrected by: [Your name]
    Date: 2025
 ****************************************************/

#ifndef STPM3X_DEFINE_h
#define STPM3X_DEFINE_h

#ifndef MASTER_CORE
// #ifdef POWER_METER_DEVICE

#define STPM3x_FRAME_LEN 5
#define CRC_8 (0x07)

/*========================================================================*/
/*                      CONFIGURATION REGISTERS (RW)                      */
/*========================================================================*/

/*------Structure definition for DSP CR1 LSW register STPM3X--------------*/
/*  Row:     0                                                            */
/*  Address: 0x00                                                         */
/*  Name:    DSP_CR1_LSW[15:0]                                            */
/*  Read(R) Write(W) Latch(L): RW                                         */
/*  Default: 0x00A0                                                       */
typedef union {
    struct {
        unsigned CLRSS_TO1 : 4;   // Clear SS timeout for channel 1
        unsigned ClearSS1  : 1;   // Clear snapshot for channel 1
        unsigned ENVREF1   : 1;   // Enable internal voltage reference for primary channel
        unsigned TC1       : 3;   // Temperature compensation coefficient for VREF1
        unsigned           : 7;   // Reserved
    };
    struct {
        unsigned LSB: 8;
        unsigned MSB: 8;
    };
} DSP_CR1_LSW_bits_t;

/*------Structure definition for DSP CR1 MSW register STPM3X--------------*/
/*  Row:     0                                                            */
/*  Address: 0x01                                                         */
/*  Name:    DSP_CR1_MSW[31:16]                                           */
/*  Default: 0x0400                                                       */
typedef union {
    struct {
        unsigned      : 1;        // Reserved
        unsigned AEM1 : 1;        // Apparent energy mode: 0=RMS, 1=vectorial
        unsigned APM1 : 1;        // Apparent power mode: 0=fundamental, 1=active
        unsigned BHPFV1 : 1;      // Bypass HPF voltage: 0=enabled, 1=bypassed
        unsigned BHPFC1 : 1;      // Bypass HPF current: 0=enabled, 1=bypassed
        unsigned ROC1   : 1;      // Rogowski integrator for current channel 1
        unsigned        : 1;      // Voltage freq content: 0=fundamental, 1=wideband
        unsigned        : 1;      // Current freq content: 0=fundamental, 1=wideband
        unsigned LPW1   : 4;      // LED1 speed dividing factor
        unsigned LPS1   : 2;      // LED1 power selection: 00=active, 01=fund, 10=react, 11=apparent
        unsigned LCS1   : 2;      // LED1 channel selection
    };
    struct {
        unsigned LSB: 8;
        unsigned MSB: 8;
    };
} DSP_CR1_MSW_bits_t;

/*------Structure definition for DSP CR2 LSW register STPM3X--------------*/
/*  Row:     1                                                            */
/*  Address: 0x02                                                         */
/*  Name:    DSP_CR2_LSW[15:0]                                            */
/*  Default: 0x00A0                                                       */
typedef union {
    struct {
        unsigned CLRSS_TO2 : 4;   // Clear SS timeout for channel 2
        unsigned ClearSS2  : 1;   // Clear snapshot for channel 2
        unsigned ENVREF2   : 1;   // Enable internal voltage reference for secondary channel
        unsigned TC2       : 3;   // Temperature compensation coefficient for VREF2
        unsigned           : 7;   // Reserved
    };
    struct {
        unsigned LSB: 8;
        unsigned MSB: 8;
    };
} DSP_CR2_LSW_bits_t;

/*------Structure definition for DSP CR2 MSW register STPM3X--------------*/
/*  Row:     1                                                            */
/*  Address: 0x03                                                         */
/*  Name:    DSP_CR2_MSW[31:16]                                           */
/*  Default: 0x0400                                                       */
typedef union {
    struct {
        unsigned        : 1;      // Reserved
        unsigned AEM2   : 1;      // Apparent energy mode: 0=RMS, 1=vectorial
        unsigned APM2   : 1;      // Apparent power mode: 0=fundamental, 1=active
        unsigned BHPFV2 : 1;      // Bypass HPF voltage: 0=enabled, 1=bypassed
        unsigned BHPFC2 : 1;      // Bypass HPF current: 0=enabled, 1=bypassed
        unsigned ROC2   : 1;      // Rogowski integrator for current channel 2
        unsigned        : 1;      // Voltage freq content: 0=fundamental, 1=wideband
        unsigned        : 1;      // Current freq content: 0=fundamental, 1=wideband
        unsigned LPW2   : 4;      // LED2 speed dividing factor
        unsigned LPS2   : 2;      // LED2 power selection
        unsigned LCS2   : 2;      // LED2 channel selection
    };

    struct {
        unsigned LSB: 8;
        unsigned MSB: 8;
    };
} DSP_CR2_MSW_bits_t;

/*------Structure definition for DSP CR3 LSW register STPM3X--------------*/
/*  Row:     2                                                            */
/*  Address: 0x04                                                         */
/*  Name:    DSP_CR3_LSW[15:0]                                            */
/*  Default: 0x04E0                                                       */
typedef union {
    struct {
        unsigned SAG_TIME_THR : 14; // Sag time threshold
        unsigned ZCR_SEL      : 2;  // Zero crossing selection
    };
    struct {
        unsigned LSB: 8;
        unsigned MSB: 8;
    };
} DSP_CR3_LSW_bits_t;

/*------Structure definition for DSP CR3 MSW register STPM3X--------------*/
/*  Row:     2                                                            */
/*  Address: 0x05                                                         */
/*  Name:    DSP_CR3_MSW[31:16]                                           */
/*  Default: 0x0000                                                       */
typedef union {
    struct {
        unsigned ZCR_EN        : 1; // ZCR/CLK pin output: 0=CLK, 1=ZCR
        unsigned TMP_TOL       : 2; // Temperature tolerance
        unsigned TMP_EN        : 1; // Temperature compensation enable
        unsigned SW_Reset      : 1; // Software reset
        unsigned SW_Latch1     : 1; // Primary channel latch
        unsigned SW_Latch2     : 1; // Secondary channel latch
        unsigned SW_Auto_Latch : 1; // Automatic latch at 7.8125 kHz
        unsigned LED_OFF1      : 1; // LED1 output disable
        unsigned LED_OFF2      : 1; // LED2 output disable
        unsigned EN_CUM        : 1; // Cumulative energy calculation mode
        unsigned REFFREQ       : 1; // Reference frequency: 0=50Hz, 1=60Hz
        unsigned               : 4; // Reserved
    };
    struct {
        unsigned LSB: 8;
        unsigned MSB: 8;
    };
} DSP_CR3_MSW_bits_t;

/*------Definition structure for DSP CR4 register STPM3X------------------*/
/*  Row:     3                                                            */
/*  Address: 0x06                                                         */
/*  Name:    DSP_CR4[31:0]                                                */
/*  Default: 0x00000000                                                   */
typedef union {
    struct {
        unsigned PHC2 : 10; // Phase current 2 calibration
        unsigned PHV2 : 2;  // Phase voltage 2 calibration
        unsigned PHC1 : 10; // Phase current 1 calibration
        unsigned PHV1 : 2;  // Phase voltage 1 calibration
        unsigned      : 8;  // Reserved
    };

    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;

        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} DSP_CR4_bits_t;

/*------Definition structure for DSP CR5 register STPM3X------------------*/
/*  Row:     4                                                            */
/*  Address: 0x08                                                         */
/*  Name:    DSP_CR5[31:0]                                                */
/*  Default: 0x003FF800                                                   */
typedef union {
    struct {
        unsigned CHV1     : 12; // Channel voltage 1 calibration
        unsigned SWV_THR1 : 10; // Swell voltage threshold 1
        unsigned SAG_THR1 : 10; // Sag threshold 1
    };
    
    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;
        
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} DSP_CR5_bits_t;

/*------Definition structure for DSP CR6 register STPM3X------------------*/
/*  Row:     5                                                            */
/*  Address: 0x0A                                                         */
/*  Name:    DSP_CR6[31:0]                                                */
/*  Default: 0x003FF800                                                   */
typedef union {
    struct {
        unsigned CHC1     : 12; // Channel current 1 calibration
        unsigned SWC_THR1 : 10; // Swell current threshold 1
        unsigned          : 10; // Reserved
    };
    
    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;
        
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} DSP_CR6_bits_t;

/*------Definition structure for DSP CR7 register STPM3X------------------*/
/*  Row:     6                                                            */
/*  Address: 0x0C                                                         */
/*  Name:    DSP_CR7[31:0]                                                */
/*  Default: 0x003FF800                                                   */
typedef union {
    struct {
        unsigned CHV2     : 12; // Channel voltage 2 calibration
        unsigned SWV_THR2 : 10; // Swell voltage threshold 2
        unsigned SAG_THR2 : 10; // Sag threshold 2
    };
    
    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;
        
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} DSP_CR7_bits_t;

/*------Definition structure for DSP CR8 register STPM3X------------------*/
/*  Row:     7                                                            */
/*  Address: 0x0E                                                         */
/*  Name:    DSP_CR8[31:0]                                                */
/*  Default: 0x003FF800                                                   */
typedef union {
    struct {
        unsigned CHC2     : 12; // Channel current 2 calibration
        unsigned SWC_THR2 : 10; // Swell current threshold 2
        unsigned          : 10; // Reserved
    };
    
    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;
        
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} DSP_CR8_bits_t;

/*------Definition structure for DSP CR9 register STPM3X------------------*/
/*  Row:     8                                                            */
/*  Address: 0x10                                                         */
/*  Name:    DSP_CR9[31:0]                                                */
/*  Default: 0x0000FF                                                     */
typedef union {
    struct {
        unsigned AH_UP1 : 12; // Ah accumulation upper threshold 1
        unsigned OFA1   : 10; // Active power offset 1
        unsigned OFAF1  : 10; // Fundamental power offset 1
    };
    
    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;
        
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} DSP_CR9_bits_t;

/*------Definition structure for DSP CR10 register STPM3X-----------------*/
/*  Row:     9                                                            */
/*  Address: 0x12                                                         */
/*  Name:    DSP_CR10[31:0]                                               */
/*  Default: 0x0000FF                                                     */
typedef union {
    struct {
        unsigned AH_DOWN1 : 12; // Ah accumulation lower threshold 1
        unsigned OFR1     : 10; // Reactive power offset 1
        unsigned OFS1     : 10; // Apparent power offset 1
    };
    
    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;
        
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} DSP_CR10_bits_t;

/*------Definition structure for DSP CR11 register STPM3X-----------------*/
/*  Row:     10                                                           */
/*  Address: 0x14                                                         */
/*  Name:    DSP_CR11[31:0]                                               */
/*  Default: 0x0000FF                                                     */
typedef union {
    struct {
        unsigned AH_UP2 : 12; // Ah accumulation upper threshold 2
        unsigned OFA2   : 10; // Active power offset 2
        unsigned OFAF2  : 10; // Fundamental power offset 2
    };
    
    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;
        
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} DSP_CR11_bits_t;

/*------Definition structure for DSP CR12 register STPM3X-----------------*/
/*  Row:     11                                                           */
/*  Address: 0x16                                                         */
/*  Name:    DSP_CR12[31:0]                                               */
/*  Default: 0x0000FF                                                     */
typedef union {
    struct {
        unsigned AH_DOWN2 : 12; // Ah accumulation lower threshold 2
        unsigned OFR2     : 10; // Reactive power offset 2
        unsigned OFS2     : 10; // Apparent power offset 2
    };
    
    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;
        
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} DSP_CR12_bits_t;

/*------Structure definition for DFE CR1 LSW register STPM3X--------------*/
/*  Row:     12                                                           */
/*  Address: 0x18                                                         */
/*  Name:    DFE_CR1_LSW[15:0]                                            */
/*  Default: 0x0F270327                                                   */
typedef union {
    struct {
        unsigned ENV1 : 1; // Enable voltage channel 1
        unsigned      : 15; // Reserved
    };

    struct {
        unsigned LSB: 8;
        unsigned MSB: 8;
    };
} DFE_CR1_LSW_bits_t;

/*------Structure definition for DFE CR1 MSW register STPM3X--------------*/
/*  Row:     12                                                           */
/*  Address: 0x19                                                         */
/*  Name:    DFE_CR1_MSW[31:16]                                           */
/*  Default: 0x0F270327                                                   */
typedef union {
    struct {
        unsigned ENC1  : 1; // Enable current channel 1
        unsigned       : 9; // Reserved
        unsigned GAIN1 : 2; // Gain selection for channel 1
        unsigned       : 4; // Reserved
    };

    struct {
        unsigned LSB: 8;
        unsigned MSB: 8;
    };
} DFE_CR1_MSW_bits_t;

/*------Structure definition for DFE CR2 LSW register STPM3X--------------*/
/*  Row:     13                                                           */
/*  Address: 0x1A                                                         */
/*  Name:    DFE_CR2_LSW[15:0]                                            */
/*  Default: 0x03270327                                                   */
typedef union {
    struct {
        unsigned ENV2 : 1; // Enable voltage channel 2
        unsigned      : 15; // Reserved
    };

    struct {
        unsigned LSB: 8;
        unsigned MSB: 8;
    };
} DFE_CR2_LSW_bits_t;

/*------Structure definition for DFE CR2 MSW register STPM3X--------------*/
/*  Row:     13                                                           */
/*  Address: 0x1B                                                         */
/*  Name:    DFE_CR2_MSW[31:16]                                           */
/*  Default: 0x03270327                                                   */
typedef union {
    struct {
        unsigned ENC2  : 1; // Enable current channel 2
        unsigned       : 9; // Reserved
        unsigned GAIN2 : 2; // Gain selection for channel 2
        unsigned       : 4; // Reserved
    };

    struct {
        unsigned LSB: 8;
        unsigned MSB: 8;
    };
} DFE_CR2_MSW_bits_t;

/*------Structure definition for DSP_IRQ1 register STPM3X-----------------*/
/*  Row:     14                                                           */
/*  Address: 0x1C                                                         */
/*  Name:    DSP_IRQ1[31:0]                                               */
/*  Default: 0x00000000                                                   */
typedef union {
    struct {
        unsigned PH1_2_IRQ_CR   : 4; 
        unsigned PH2_IRQ_CR     : 8;  
        unsigned PH1_IRQ_CR     : 8;
        unsigned C1_IRQ_CR      : 4;
        unsigned V1_IRQ_CR      : 6;
        unsigned TAMPER         : 2;
    };
    
    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;
        
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} DSP_IRQ1_bits_t;

/*------Structure definition for DSP_IRQ2 register STPM3X-----------------*/
/*  Row:     15                                                           */
/*  Address: 0x1E                                                         */
/*  Name:    DSP_IRQ2[31:0]                                               */
/*  Default: 0x00000000                                                   */
typedef union {
    struct {
        unsigned PH1_2_IRQ_CR   : 4; 
        unsigned PH2_IRQ_CR     : 8;  
        unsigned PH1_IRQ_CR     : 8;
        unsigned C1_IRQ_CR      : 4;
        unsigned V1_IRQ_CR      : 6;
        unsigned TAMPER         : 2;
    };
    
    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;
        
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} DSP_IRQ2_bits_t;

/*========================================================================*/
/*                      STATUS REGISTERS (R/RWL)                          */
/*========================================================================*/

/*------Structure definition for DSP SR1 register STPM3X------------------*/
/*  Row:     16                                                           */
/*  Address: 0x20                                                         */
/*  Name:    DSP_SR1[15:0]                                                */
/*  Default: 0x00000000                                                   */
typedef union {
    struct {
        unsigned signTotalActivePower : 1;							// Sign total active power
        unsigned signTotalReactivePower : 1;						// Sign total reactive power
        unsigned overflowTotalActiveEnergy : 1;						// Overflow total active energy
        unsigned overflowTotalReactiveEnergy : 1;					// Overflow total reactive energy

                                                                    // == PH2 IRQ status ==
        unsigned signSecondaryChannelActivePower : 1;				// Sign secondary channel active power
        unsigned signSecondaryChannelActiveFundamentalPower : 1;	// Sign secondary channel active fundamental power
        unsigned signSecondaryChannelReactivePower : 1;				// Sign secondary channel reactive power
        unsigned signSecondaryChannelApparentPower : 1;				// Sign secondary channel apparent power
        unsigned overflowSecondaryChannelActiveEnergy : 1;			// Overflow secondary channel active energy
        unsigned overflowSecondaryChannelActiveFundamentalEnergy: 1;// Overflow secondary channel active fundamental energy
        unsigned overflowSecondaryChannelReactiveEnergy : 1;		// Overflow secondary channel reactive energy
        unsigned overflowSecondaryChannelApparentEnergy : 1;		// Overflow secondary channel apparent energy

                                                                    // == PH1 IRQ status ==
        unsigned signPrimaryChannelActivePower : 1;					// Sign primary channel active power
        unsigned signPrimaryChannelActiveFundamentalPower : 1;		// Sign primary channel active fundamental power
        unsigned signPrimaryChannelReactivePower : 1;				// Sign primary channel reactive power
        unsigned signPrimaryChannelApparentPower : 1;				// Sign primary channel apparent power
        unsigned overflowPrimaryChannelActiveEnergy : 1;			// Overflow primary channel active energy
        unsigned overflowPrimaryChannelActiveFundamentalEnergy : 1;	// Overflow primary channel active fundamental energy
        unsigned overflowPrimaryChannelReactiveEnergy : 1;			// Overflow primary channel reactive energy
        unsigned overflowPrimaryChannelApparentEnergy : 1;			// Overflow primary channel apparent energy

                                                                    // == C1 IRQ status ==
        unsigned primaryCurrentSigmaDeltaBitstreamStuck : 1;		// Primary current sigma - delta bitstream stuck
        unsigned ah1AccumulationOfPrimaryChannelCurrent : 1;		// AH1 - accumulation of primary channel current
        unsigned primaryCurrentSwellDetected : 1;					// Primary current swell detected
        unsigned primaryCurrentSwellEnd : 1;						// Primary current swell end

                                                                    // == V1 IRQ status ==
        unsigned primaryVoltageSigmaDeltaBitstreamStuck : 1;		// Primary voltage sigma - delta bitstream stuck
        unsigned primaryVoltagePeriodError : 1;						// Primary voltage period error
        unsigned primaryVoltageSagDetected : 1;						// Primary voltage sag detected
        unsigned primaryVoltageSagEnd : 1;							// Primary voltage sag end
        unsigned primaryVoltageSwellDetected : 1;					// Primary voltage swell detected
        unsigned primaryVoltageSwellEnd : 1;						// Primary voltage swell end

                                                                    // == Tamper ==
        unsigned tamperOnPrimary : 1;								// Tamper on primary
        unsigned tamperOrWrongConnection : 1;						// Tamper or wrong connection
   
    };
    
    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;
        
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} DSP_SR1_bits_t;

/*------Structure definition for DSP SR2 register STPM3X------------------*/
/*  Row:     17                                                           */
/*  Address: 0x22                                                         */
/*  Name:    DSP_SR2[15:0]                                                */
/*  Default: 0x00000000                                                   */
typedef union {
    struct {
        unsigned signTotalActivePower : 1;							// Sign total active power
        unsigned signTotalReactivePower : 1;						// Sign total reactive power
        unsigned overflowTotalActiveEnergy : 1;						// Overflow total active energy
        unsigned overflowTotalReactiveEnergy : 1;					// Overflow total reactive energy

                                                                    // == PH2 status ==
        unsigned signSecondaryChannelActivePower : 1;				// Sign secondary channel active power
        unsigned signSecondaryChannelActiveFundamentalPower : 1;	// Sign secondary channel active fundamental power
        unsigned signSecondaryChannelReactivePower : 1;				// Sign secondary channel reactive power
        unsigned signSecondaryChannelApparentPower : 1;				// Sign secondary channel apparent power
        unsigned overflowSecondaryChannelActiveEnergy : 1;			// Overflow secondary channel active energy
        unsigned overflowSecondaryChannelActiveFundamentalEnergy : 1;// Overflow secondary channel active fundamental energy
        unsigned overflowSecondaryChannelReactiveEnergy : 1;		// Overflow secondary channel reactive energy
        unsigned overflowSecondaryChannelApparentEnergy : 1;		// Overflow secondary channel apparent energy

                                                                    // == PH1 status ==
        unsigned signPrimaryChannelActivePower : 1;					// Sign primary channel active power
        unsigned signPrimaryChannelActiveFundamentalPower : 1;		// Sign primary channel active fundamental power
        unsigned signPrimaryChannelReactivePower : 1;				// Sign primary channel reactive power
        unsigned signPrimaryChannelApparentPower : 1;				// Sign primary channel apparent power
        unsigned overflowPrimaryChannelActiveEnergy : 1;			// Overflow primary channel active energy
        unsigned overflowPrimaryChannelActiveFundamentalEnergy : 1;	// Overflow primary channel active fundamental energy
        unsigned overflowPrimaryChannelReactiveEnergy : 1;			// Overflow primary channel reactive energy
        unsigned overflowPrimaryChannelApparentEnergy : 1;			// Overflow primary channel apparent energy

                                                                    // == C2 status ==
        unsigned secondaryCurrentSigmaDeltaBitstreamStuck : 1;		// Secondary current sigma - delta bitstream stuck
        unsigned ah1AccumulationOfSecondaryChannelCurrent : 1;		// AH1 - accumulation of secondary channel current
        unsigned secondaryCurrentSwellDetected : 1;					// Secondary current swell detected
        unsigned secondaryCurrentSwellEnd : 1;						// Secondary current swell end

                                                                    // == V2 status ==
        unsigned secondaryVoltageSigmaDeltaBitstreamStuck : 1;		// Secondary voltage sigma - delta bitstream stuck
        unsigned secondaryVoltagePeriodError : 1;					// Secondary voltage period error
        unsigned secondaryVoltageSagDetected : 1;					// Secondary voltage sag detected
        unsigned secondaryVoltageSagEnd : 1;						// Secondary voltage sag end
        unsigned secondaryVoltageSwellDetected : 1;					// Secondary voltage swell detected
        unsigned secondaryVoltageSwellEnd : 1;						// Secondary voltage swell end

                                                                    // == Tamper ==
        unsigned tamperOnSecondary : 1;								// Tamper on secondary
        unsigned tamperOrWrongConnection : 1;						// Tamper or wrong connection
    };
    struct {
        unsigned LSB: 8;
        unsigned MSB: 8;
    };
} DSP_SR2_bits_t;

/*------Structure definition for US_REG1 LSW register STPM3X--------------*/
/*  Row:     18                                                           */
/*  Address: 0x24                                                         */
/*  Name:    US_REG1_LSW[15:0]                                            */
/*  Default: 0x4007                                                       */
typedef union {
    struct {
        unsigned CRC_Pol    : 8;  // CRC polynomial
        unsigned Noise_EN   : 1;  // Noise enable
        unsigned BRK_ERR    : 1;  // Break error
        unsigned            : 4;  // Reserved
        unsigned CRC_EN     : 1;  // CRC enable
        unsigned LSB_FIRST  : 1;  // LSB first transmission
    };

    struct {
        unsigned LSB: 8;
        unsigned MSB: 8;
    };
} US_REG1_LSW_bits_t;

/*------Structure definition for US_REG1 MSW register STPM3X--------------*/
/*  Row:     18                                                           */
/*  Address: 0x25                                                         */
/*  Name:    US_REG1_MSW[31:16]                                           */
/*  Default: 0x00000000                                                   */
typedef union {
    struct {
        unsigned TIME_OUT : 8; // Timeout value (ms)
        unsigned          : 8; // Reserved
    };

    struct {
        unsigned LSB: 8;
        unsigned MSB: 8;
    };
} US_REG1_MSW_bits_t;

/*------Structure definition for US_REG2 register STPM3X------------------*/
/*  Row:     19                                                           */
/*  Address: 0x26                                                         */
/*  Name:    US_REG2[31:0]                                                */
/*  Default: 0x00000683                                                   */
typedef union {
    struct {
        unsigned BAUD_RATE   : 16; // Baud rate setting
        unsigned FRAME_DELAY : 8; // Frame delay
        unsigned             : 8; // Reserved
    };
    
    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;
        
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} US_REG2_bits_t;

/*------Structure definition for US_REG3 register STPM3X------------------*/
/*  Row:     20                                                           */
/*  Address: 0x28                                                         */
/*  Name:    US_REG3[15:0] - UART & SPI IRQ Status/Control                */
/*  Default: 0x00000000                                                   */
typedef union {
    struct {
        unsigned                        : 1;  // Reserved
        unsigned UART_CRC_error         : 1;  // UART CRC error
        unsigned UART_timeout_error     : 1;  // UART timeout error
        unsigned UART_framing_error     : 1;  // UART framing error
        unsigned UART_noise_error       : 1;  // UART noise error
        unsigned UART_RX_overrun        : 1;  // UART RX overrun
        unsigned UART_TX_overrun        : 1;  // UART TX overrun
        unsigned                        : 1;  // Reserved
        unsigned SPI_RX_full            : 1;  // SPI RX full 
        unsigned SPITX_empty            : 1;  // SPITX empty
        unsigned UART_SPI_read_error    : 1;  // UART/SPI read error
        unsigned UART_SPI_write_error   : 1;  // UART/SPI write error
        unsigned SPI_CRC_error          : 1;  // SPI CRC error
        unsigned SPI_TX_underrun        : 1;  // SPI TX underrun 
        unsigned SPI_RX_overrun         : 1;  // SPI RX overrun
        unsigned                        : 1;  // Reserved
    };
    
    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;
        
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} US_REG3_LSB_bits_t;

/*========================================================================*/
/*                      DATA REGISTERS (RL)                               */
/*========================================================================*/

/*------Structure definition for DSP_EV1 register STPM3X------------------*/
/*  Row:     21                                                           */
/*  Address: 0x2A                                                         */
/*  Name:    DSP_EV1[31:0] - Event Status 1                               */
/*  Default: 0x00000000                                                   */
typedef union {
    struct {
        unsigned SAG1_EV    : 4;  // Sag event channel 1
        unsigned SWV1_EV    : 4;  // Swell voltage event 1
        unsigned           : 4;  // Reserved
        unsigned SWC1_EV    : 4;  // Swell current event 1
        unsigned Nah        : 1;  // Nah event
        unsigned           : 15; // Reserved
    };
    
    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;
        
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} DSP_EV1_bits_t;

/*------Structure definition for DSP_EV2 register STPM3X------------------*/
/*  Row:     22                                                           */
/*  Address: 0x2C                                                         */
/*  Name:    DSP_EV2[31:0] - Event Status 2                               */
/*  Default: 0x00000000                                                   */
typedef union {
    struct {
        unsigned SAG2_EV : 4;  // Sag event channel 2
        unsigned SWV2_EV : 4;  // Swell voltage event 2
        unsigned         : 4;  // Reserved
        unsigned SWC2_EV : 4;  // Swell current event 2
        unsigned         : 16; // Reserved
    };
    
    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;
        
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} DSP_EV2_bits_t;

/*------Structure definition for DSP_REG1 register STPM3X-----------------*/
/*  Row:     23                                                           */
/*  Address: 0x2E                                                         */
/*  Name:    DSP_REG1[31:0] - Period data                                 */
/*  Default: 0x00000000                                                   */
typedef union {
    struct {
        unsigned PH2_PERIOD : 16; // Phase 2 period
        unsigned PH1_PERIOD : 16; // Phase 1 period
    };
    
    struct {
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } LSW;
        
        struct {
            unsigned LSB: 8;
            unsigned MSB: 8;
        } MSW;
    };
} DSP_REG1_bits_t;

/*------Simple register structures for data registers--------------------*/
typedef struct {
    unsigned int   DATA;
    unsigned short Address;
} DSP_DATA_REG_t;

/*========================================================================*/
/*                      REGISTER ADDRESS DEFINITIONS                      */
/*========================================================================*/

// Configuration Registers (RW)
#define DSP_CR1_Address                 0x00
#define DSP_CR2_Address                 0x02
#define DSP_CR3_Address                 0x04
#define DSP_CR4_Address                 0x06
#define DSP_CR5_Address                 0x08
#define DSP_CR6_Address                 0x0A
#define DSP_CR7_Address                 0x0C
#define DSP_CR8_Address                 0x0E
#define DSP_CR9_Address                 0x10
#define DSP_CR10_Address                0x12
#define DSP_CR11_Address                0x14
#define DSP_CR12_Address                0x16
#define DFE_CR1_Address                 0x18
#define DFE_CR2_Address                 0x1A
#define DSP_IRQ1_Address                0x1C
#define DSP_IRQ2_Address                0x1E

// Status Registers (RWL)
#define DSP_SR1_Address                 0x20
#define DSP_SR2_Address                 0x22

// UART/SPI Registers (RW)
#define US_REG1_Address                 0x24
#define US_REG2_Address                 0x26
#define US_REG3_Address                 0x28

// Event Registers (RL)
#define DSP_EV1_Address                 0x2A
#define DSP_EV2_Address                 0x2C

// Data Registers (RL)
#define DSP_REG1_Address                0x2E  // Period data (PH2[15:0], PH1[15:0])
#define V1_Data_Address                 0x30  // V1 instantaneous data [23:0]
#define C1_Data_Address                 0x32  // C1 instantaneous data [23:0]
#define V2_Data_Address                 0x34  // V2 instantaneous data [23:0]
#define C2_Data_Address                 0x36  // C2 instantaneous data [23:0]
#define V1_Fund_Address                 0x38  // V1 fundamental [23:0]
#define C1_Fund_Address                 0x3A  // C1 fundamental [23:0]
#define V2_Fund_Address                 0x3C  // V2 fundamental [23:0]
#define C2_Fund_Address                 0x3E  // C2 fundamental [23:0]

// RMS Data Registers (RL)
#define DSP_REG14_Address               0x48  // C1_RMS[16:0], V1_RMS[14:0]
#define DSP_REG15_Address               0x4A  // C2_RMS[16:0], V2_RMS[14:0]

// Sag/Swell Time Registers (RL)
#define DSP_REG16_Address               0x4C  // SAG1_TIME[14:0], SWV1_TIME[14:0]
#define DSP_REG17_Address               0x4E  // C1PHA[11:0], SWC1_TIME[14:0]
#define DSP_REG18_Address               0x50  // SAG2_TIME[14:0], SWV2_TIME[14:0]
#define DSP_REG19_Address               0x52  // C2PHA[11:0], SWC2_TIME[14:0]

// Phase 1 Energy Registers (RL)
#define PH1_Active_Energy_Address       0x54  // PH1 Active Energy [31:0]
#define PH1_Fundamental_Energy_Address  0x56  // PH1 Fundamental Energy [31:0]
#define PH1_Reactive_Energy_Address     0x58  // PH1 Reactive Energy [31:0]
#define PH1_Apparent_Energy_Address     0x5A  // PH1 Apparent Energy [31:0]

// Phase 1 Power Registers (RL)
#define PH1_Active_Power_Address        0x5C  // PH1 Active Power [28:0]
#define PH1_Fundamental_Power_Address   0x5E  // PH1 Fundamental Power [28:0]
#define PH1_Reactive_Power_Address      0x60  // PH1 Reactive Power [28:0]
#define PH1_Apparent_RMS_Power_Address  0x62  // PH1 Apparent RMS Power [28:0]
#define PH1_Apparent_Vec_Power_Address  0x64  // PH1 Apparent Vectorial Power [28:0]
#define PH1_Momentary_Active_Power_Address  0x66  // PH1 Momentary Active Power [28:0]
#define PH1_Momentary_Fund_Power_Address    0x68  // PH1 Momentary Fundamental Power [28:0]
#define PH1_AH_ACC_Address              0x6A  // PH1 Ah Accumulator

// Phase 2 Energy Registers (RL)
#define PH2_Active_Energy_Address       0x6C  // PH2 Active Energy [31:0]
#define PH2_Fundamental_Energy_Address  0x6E  // PH2 Fundamental Energy [31:0]
#define PH2_Reactive_Energy_Address     0x70  // PH2 Reactive Energy [31:0]
#define PH2_Apparent_Energy_Address     0x72  // PH2 Apparent RMS Energy [31:0]

// Phase 2 Power Registers (RL)
#define PH2_Active_Power_Address        0x74  // PH2 Active Power [28:0]
#define PH2_Fundamental_Power_Address   0x76  // PH2 Fundamental Power [28:0]
#define PH2_Reactive_Power_Address      0x78  // PH2 Reactive Power [28:0]
#define PH2_Apparent_RMS_Power_Address  0x7A  // PH2 Apparent RMS Power [28:0]
#define PH2_Apparent_Vec_Power_Address  0x7C  // PH2 Apparent Vectorial Power [28:0]
#define PH2_Momentary_Active_Power_Address  0x7E  // PH2 Momentary Active Power [28:0]
#define PH2_Momentary_Fund_Power_Address    0x80  // PH2 Momentary Fundamental Power [28:0]
#define PH2_AH_ACC_Address              0x82  // PH2 Ah Accumulator

// Total Energy Registers (RL)
#define Total_Active_Energy_Address     0x84  // Total Active Energy [31:0]
#define Total_Fundamental_Energy_Address 0x86  // Total Fundamental Energy [31:0]
#define Total_Reactive_Energy_Address   0x88  // Total Reactive Energy [31:0]
#define Total_Apparent_Energy_Address   0x8A  // Total Apparent Energy [31:0]

/*========================================================================*/
/*                      PARAMETER REGISTER STRUCTURE                      */
/*========================================================================*/

typedef struct {
    unsigned int*   dataBuffer;
    unsigned short  Row;
    unsigned short  Address;
} STPM3x_Param_Reg_t;

/*========================================================================*/
/*                      UTILITY MACROS                                    */
/*========================================================================*/

// Energy and power calculation constants
#define ENERGY_UPDATE_MS        100
#define NOISE_POWER             0.1
#define MAX_POWER               4000.0
#define ENERGY_LSB              0.00000000886162

// Bit manipulation macros
#define SET_BIT(REG, BIT)       ((REG) |= (1U << (BIT)))
#define CLEAR_BIT(REG, BIT)     ((REG) &= ~(1U << (BIT)))
#define READ_BIT(REG, BIT)      (((REG) >> (BIT)) & 1U)
#define TOGGLE_BIT(REG, BIT)    ((REG) ^= (1U << (BIT)))

// Register access type markers
#define REG_RW      0  // Read/Write
#define REG_RL      1  // Read/Latch
#define REG_RWL     2  // Read/Write/Latch

/*========================================================================*/
/*                      REGISTER BIT POSITIONS                            */
/*========================================================================*/

// DSP_CR1 MSW bit positions
#define DSP_CR1_AEM1_POS        1
#define DSP_CR1_APM1_POS        2
#define DSP_CR1_BHPFV1_POS      3
#define DSP_CR1_BHPFC1_POS      4
#define DSP_CR1_ROC1_POS        5
#define DSP_CR1_BLPFV1_POS      6
#define DSP_CR1_BLPFC1_POS      7

// DSP_CR3 MSW bit positions
#define DSP_CR3_ZCR_EN_POS      0
#define DSP_CR3_TMP_EN_POS      3
#define DSP_CR3_SW_RESET_POS    4
#define DSP_CR3_SW_LATCH1_POS   5
#define DSP_CR3_SW_LATCH2_POS   6
#define DSP_CR3_AUTO_LATCH_POS  7
#define DSP_CR3_LED_OFF1_POS    8
#define DSP_CR3_LED_OFF2_POS    9
#define DSP_CR3_EN_CUM_POS      10
#define DSP_CR3_REFFREQ_POS     11

// DFE_CR bit positions
#define DFE_CR_ENV_POS          0
#define DFE_CR_ENC_POS          0
#define DFE_CR_GAIN_POS         10

/*========================================================================*/
/*                      GAIN SETTINGS                                     */
/*========================================================================*/

typedef enum {
    STPM3X_GAIN_2X  = 0,  // Gain = 2
    STPM3X_GAIN_4X  = 1,  // Gain = 4
    STPM3X_GAIN_8X  = 2,  // Gain = 8
    STPM3X_GAIN_16X = 3   // Gain = 16
} STPM3x_Gain_t;

/*========================================================================*/
/*                      LED POWER SELECTION                               */
/*========================================================================*/

typedef enum {
    LED_POWER_ACTIVE      = 0,  // Active power
    LED_POWER_FUNDAMENTAL = 1,  // Fundamental power
    LED_POWER_REACTIVE    = 2,  // Reactive power
    LED_POWER_APPARENT    = 3   // Apparent power
} STPM3x_LED_Power_t;

/*========================================================================*/
/*                      LED CHANNEL SELECTION                             */
/*========================================================================*/

typedef enum {
    LED_CHANNEL_PRIMARY   = 0,  // Primary channel
    LED_CHANNEL_SECONDARY = 1,  // Secondary channel
    LED_CHANNEL_CUMULATIVE = 2, // Cumulative (both channels)
    LED_CHANNEL_BITSTREAM = 3   // Sigma-delta bitstream
} STPM3x_LED_Channel_t;

/*========================================================================*/
/*                      REFERENCE FREQUENCY                               */
/*========================================================================*/

typedef enum {
    STPM3X_FREQ_50HZ = 0,  // 50 Hz
    STPM3X_FREQ_60HZ = 1   // 60 Hz
} STPM3x_Ref_Freq_t;

/*========================================================================*/
/*                      TEMPERATURE COMPENSATION                          */
/*========================================================================*/

typedef enum {
    STPM3X_TC_NONE    = 0,  // No temperature compensation
    STPM3X_TC_LOW     = 1,  // Low temperature coefficient
    STPM3X_TC_MEDIUM  = 2,  // Medium temperature coefficient
    STPM3X_TC_HIGH    = 3   // High temperature coefficient
} STPM3x_Temp_Comp_t;

/*========================================================================*/
/*                      HELPER FUNCTIONS PROTOTYPES                       */
/*========================================================================*/

// #endif /* POWER_METER_DEVICE*/
#endif /* MASTER_CORE*/
#endif /* STPM3X_DEFINE_h */
