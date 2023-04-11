// Tell emacs that this is a C++ source
//  -*- C++ -*-.
#ifndef G4TPC_PHG4TPCDIGITIZER_H
#define G4TPC_PHG4TPCDIGITIZER_H

#include <fun4all/SubsysReco.h>

#include <trackbase/TrkrDefs.h>
#include <trackbase/TrkrHitSet.h>

#include <map>
#include <string>   // for string
#include <utility>  // for pair, make_pair
#include <vector>

#include <gsl/gsl_rng.h>

class PHCompositeNode;

class PHG4TpcDigitizer : public SubsysReco
{
 public:
  PHG4TpcDigitizer(const std::string &name = "PHG4TpcDigitizer");
  ~PHG4TpcDigitizer() override;

  //! module initialization
  int Init(PHCompositeNode * /*topNode*/) override { return 0; }

  //! run initialization
  int InitRun(PHCompositeNode *topNode) override;

  //! event processing
  int process_event(PHCompositeNode *topNode) override;

  //! end of process
  int End(PHCompositeNode * /*topNode*/) override { return 0; };

  void set_adc_scale(const int layer, const unsigned int max_adc, const float energy_per_adc)
  {
    _max_adc.insert(std::make_pair(layer, max_adc));
    _energy_scale.insert(std::make_pair(layer, energy_per_adc));
  }

  void SetADCThreshold(const float thresh) { ADCThreshold = thresh; };
  void SetENC(const float enc) { TpcEnc = enc; };
  void set_drift_velocity(float vd) {_drift_velocity = vd;}
  void set_skip_noise_flag(const bool skip) {skip_noise = skip;}
  void set_nPreSample(unsigned int s) {m_nPreSample = s;}
  void set_nPostSample(unsigned int s) {m_nPostSample = s;}

  //! Preserve old interface
  void SetTpcMinLayer(const int minlayer);

 private:
  void CalculateCylinderCellADCScale(PHCompositeNode *topNode);
  void DigitizeCylinderCells(PHCompositeNode *topNode);
  float added_noise();
  float add_noise_to_bin(float signal);
  
  float ADCThreshold;
  float ADCThreshold_mV = 0;
  float TpcEnc;
  float Pedestal;
  float ChargeToPeakVolts;
  float _drift_velocity = 8.0e-3;  // override from macro with simulation drift velocity

  float ADCSignalConversionGain;
  float ADCNoiseConversionGain;
  //! SAMPA zero-suppression digitization parameter
  unsigned int m_nPreSample = 2;
  unsigned int m_nPostSample = 3;

  bool skip_noise = false;

  // settings
  std::map<int, unsigned int> _max_adc;
  std::map<int, float> _energy_scale;

  //! random generator that conform with sPHENIX standard
  gsl_rng *RandomGenerator;
};

#endif
