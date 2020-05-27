/**
 * @file
 * @brief Implementation of charge sensitive amplifier digitization module
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "CSADigitizerModule.hpp"

#include "core/utils/unit.h"
#include "tools/ROOT.h"

#include <TFile.h>
#include <TGraph.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>

#include "objects/PixelHit.hpp"

using namespace allpix;

CSADigitizerModule::CSADigitizerModule(Configuration& config,
                                               Messenger* messenger,
                                               std::shared_ptr<Detector> detector)
    : Module(config, std::move(detector)), messenger_(messenger), pixel_message_(nullptr) {
    // Enable parallelization of this module if multithreading is enabled
    enable_parallelization();

    // Require PixelCharge message for single detector
    messenger_->bindSingle(this, &CSADigitizerModule::pixel_message_, MsgFlags::REQUIRED);

    // Seed the random generator with the global seed
    random_generator_.seed(getRandomSeed());

    // Set defaults for config variables
    config_.setDefault<bool>("simple_parametrisation", true);
    // defaults for the "advanced" parametrisation
    config_.setDefault<double>("rise_time_constant", Units::get(1e-9, "s"));  
    config_.setDefault<double>("feedback_time_constant", Units::get(10e-9, "s"));  // R_f * C_f
    // for both cases
    config_.setDefault<double>("feedback_capacitance", Units::get(5e-15, "C/V"));
    // and for the advanced one
    config_.setDefault<double>("krummenacher_current", Units::get(20e-9, "C/s"));
    config_.setDefault<double>("detector_capacitance", Units::get(100e-15, "C/V"));
    config_.setDefault<double>("amp_output_capacitance", Units::get(20e-15, "C/V"));
    config_.setDefault<double>("transconductance", Units::get(50e-6, "C/s/V") );
    config_.setDefault<double>("v_temperature", Units::get(25.7e-3, "eV"));  // Boltzmann kT at 298K

    config_.setDefault<double>("amp_time_window", Units::get(0.5e-6, "s"));
    config_.setDefault<double>("threshold", Units::get(10e-3, "V"));
    config_.setDefault<double>("sigma_noise", Units::get(0.1e-3, "V"));
    
    config_.setDefault<bool>("output_pulsegraphs", false);
    config_.setDefault<bool>("output_plots", config_.get<bool>("output_pulsegraphs"));
    config_.setDefault<int>("output_plots_scale", Units::get(30, "ke"));
    config_.setDefault<int>("output_plots_bins", 100);


    // Copy some variables from configuration to avoid lookups:
    tmax_ = config_.get<double>("amp_time_window");
    if (config_.get<bool>("simple_parametrisation")){
      tauF_ = config_.get<double>("feedback_time_constant");
      tauR_ = config_.get<double>("rise_time_constant");
      cf_ = config_.get<double>("feedback_capacitance");
      rf_=tauF_/cf_; 
      LOG(DEBUG) << "Parameters: cf " << Units::display(cf_/1e-15, "C/V")  << ", rf " << Units::display(rf_, "V*s/C")
		 << ", tauF_ " << Units::display(tauF_, "s") << ", tauR_ " << Units::display(tauR_, "s");
    }
    else{
      ikrum_ = config_.get<double>("krummenacher_current");
      cd_ = config_.get<double>("detector_capacitance");
      cf_ = config_.get<double>("feedback_capacitance");
      co_ = config_.get<double>("amp_output_capacitance");
      gm_ = config_.get<double>("transconductance"); 
      vt_ = config_.get<double>("v_temperature");

      // helper variables: transconductance and resistance in the feedback loop 
      // weak inversion: gf = I/(n V_t) (e.g. Binkley "Tradeoff and Optimisation in Analog CMOS design")
      // n_wi typically 1.5, for circuit descriped in  Kleczek 2016 JINST11 C12001: I->I_krumm/2
      gf_ = ikrum_/(2.0*1.5*vt_)  ;  
      rf_ = 2./gf_;       //feedback resistor
      tauF_=rf_*cf_;
      tauR_=(cd_*co_)/(gm_ *cf_);
      LOG(DEBUG) << "Parameters: rf " << Units::display(rf_, "V*s/C") << ", cf_ " << Units::display(cf_, "C/V")
		 << ", cd_ " << Units::display(cd_, "C/V") << ", co_ " << Units::display(co_, "C/V")
		 << ", gm_ " << Units::display(gm_, "C/s/V") << ", tauF_ " << Units::display(tauF_, "s")
		 << ", tauR_ " << Units::display(tauR_, "s");      
    }
    
    output_plots_ = config_.get<bool>("output_plots");
    output_pulsegraphs_ = config_.get<bool>("output_pulsegraphs");


    // impulse response of the CSA from Kleczek 2016 JINST11 C12001
    // H(s) = Rf / ((1+ tau_f s) * (1 + tau_r s)), with
    // tau_f = Rf Cf , rise time constant tau_r = (C_det * C_out) / ( gm_ * C_F )
    // inverse Laplace transform R/((1+a*s)*(1+s*b)) is (wolfram alpha) (R (e^(-t/a) - e^(-t/b))) /(a - b) 
    fImpulseResponse_ = new TF1("fImpulseResponse_", "[0] * ( exp(-x / [1]) - exp(-x/[2]) ) / ([1]-[2])", 0, tmax_);
    fImpulseResponse_->SetParameters(rf_, tauF_, tauR_);


}



void CSADigitizerModule::init() {

    if(config_.get<bool>("output_plots")) {
        LOG(TRACE) << "Creating output plots";

        // Plot axis are in kilo electrons - convert from framework units!
        int maximum = static_cast<int>(Units::convert(config_.get<int>("output_plots_scale"), "ke"));
        auto nbins = config_.get<int>("output_plots_bins");

	//asv do these histos make sense? what about one with the noise?
        // Create histograms if needed
        h_pxq = new TH1D("pixelcharge", "raw pixel charge;pixel charge [ke];pixels", nbins, 0, maximum);
        h_tot = new TH1D("tot", "time over threshold;time over threshold [ns];pixels", nbins, 0, maximum);
        h_toa = new TH1D("toa", "time of arrival;time of arrival [ns];pixels", nbins, 0, maximum);
	h_pxq_vs_tot = new TH2D("pxqvstot", "ToT vs raw pixel charge;pixel charge [ke];ToT [ns]", nbins, 0, maximum, nbins, 0, tmax_);

    }

    
}



void CSADigitizerModule::run(unsigned int event_num) {
    // Loop through all pixels with charges
    std::vector<PixelHit> hits;
    for(auto& pixel_charge : pixel_message_->getData()) {
        auto pixel = pixel_charge.getPixel();
        auto pixel_index = pixel.getIndex();
        auto inputcharge = static_cast<double>(pixel_charge.getCharge());

        LOG(DEBUG) << "Received pixel " << pixel_index << ", charge " << Units::display(inputcharge, "e");

	const auto& pulse = pixel_charge.getPulse(); // the pulse containing charges and times
	auto pulse_vec = pulse.getPulse();           // the vector of the charges 
        auto timestep = pulse.getBinning();
	const int npx = static_cast<int>(ceil(tmax_/timestep));

	if (first_event_){ // initialize impulse response function - assume all time bins are equal
	  impulseResponse_ = new double[npx];
	  for (int ipx=0; ipx<npx; ++ipx){
	    impulseResponse_[ipx]=fImpulseResponse_->Eval(ipx*timestep);
	  }
	  first_event_ = false;
	  LOG(TRACE) << "impulse response initalised. timestep  : " <<  timestep << ", tmax_ : " <<  tmax_ << ", npx " << npx;
	}
	
	Pulse output_pulse(timestep);
	auto input_length = pulse_vec.size();
	LOG(TRACE) << "Preparing pulse for pixel " << pixel_index << ", " << pulse_vec.size() << " bins of "
		   << Units::display(timestep, {"ps", "ns"})
		   << ", total charge: " << Units::display(pulse.getCharge(), "e");
	//convolution of the pulse (size input_length) with the impulse response (size npx)
	for(unsigned long int k=0; k<npx; ++k){
	  for(unsigned long int i=0; i<=k; ++i){
	    if( (k-i) < input_length){
	      //asv to do: not a charge, but voltage pulse... still use this?
	      output_pulse.addCharge(pulse_vec.at(k-i) * impulseResponse_[i], timestep * static_cast<double>(k));
	    }
	  }
	}
	auto output_vec = output_pulse.getPulse(); 
	LOG(TRACE) << "amplified signal" << output_pulse.getCharge()/1e9
		   << " mV in a pulse with " << output_vec.size() << "bins";


	
	//TOA and TOT logic  
	auto threshold = config_.get<double>("threshold");
	bool is_over_threshold=false;
	double toa, tot;
	for(long unsigned int i=0; i<output_vec.size(); ++i){

	  //use while instead of for maybe?
	  // set time of arrival when pulse first crosses threshold, start countint ToT
	  if (output_vec.at(i)>threshold && !is_over_threshold){
	    is_over_threshold = true;
	    toa= i * timestep;
	    tot=timestep;
	    LOG(TRACE) << "now starting the tot counting...";
	  };
	  // asv need to add some precision to avoid noise stopping tot
	  // or do TOT/TOA before adding noise?
	  if (is_over_threshold){
	    if (output_vec.at(i)>threshold){	      
	      tot+=timestep;
	    }
	    else{
	      is_over_threshold=false;
	      break;
	    }
	  }
	  else if (output_vec.size()-1 == i){ // pulse over, never crossed the threshold
	    toa=0;
	    tot=0;
	    LOG(TRACE) << " - never crossing the threshold";
	  }
	}
 	LOG(TRACE) << "ToA " << toa << " ns, ToT " << tot << "ns";

	

	//apply noise on the amplified pulse
	std::normal_distribution<double> pulse_smearing(0, config_.get<double>("sigma_noise"));
	std::vector<double> output_with_noise(output_vec.size());
	std::transform(output_vec.begin(), output_vec.end(), output_with_noise.begin(), [&pulse_smearing, this](auto& c){return c+pulse_smearing(random_generator_);});


        if(config_.get<bool>("output_plots")) {
            h_pxq->Fill(inputcharge / 1e3);
	    h_tot->Fill(tot);
	    h_toa->Fill(toa);
	    h_pxq_vs_tot->Fill(inputcharge / 1e3,tot);
        }

	
        // Fill a graphs with the individual pixel pulses:
        if(output_pulsegraphs_) {
            // Generate x-axis:
            std::vector<double> time(pulse_vec.size());
            // clang-format off
            std::generate(time.begin(), time.end(), [n = 0.0, timestep]() mutable {  auto now = n; n += timestep; return now; });
            // clang-format on

            std::string name =
                "pulse_ev" + std::to_string(event_num) + "_px" + std::to_string(pixel_index.x()) + "-" + std::to_string(pixel_index.y());
            auto pulse_graph = new TGraph(static_cast<int>(pulse_vec.size()), &time[0], &pulse_vec[0]);
            pulse_graph->GetXaxis()->SetTitle("t [ns]");
            pulse_graph->GetYaxis()->SetTitle("Q_{ind} [e]");
            pulse_graph->SetTitle(("Induced charge in pixel (" + std::to_string(pixel_index.x()) + "," +
                                   std::to_string(pixel_index.y()) +
                                   "), Q_{tot} = " + std::to_string(pulse.getCharge()) + " e")
                                      .c_str());
            getROOTDirectory()->WriteTObject(pulse_graph, name.c_str());

            // Generate graphs of integrated charge over time:
            std::vector<double> charge_vec;
            double charge = 0;
            for(const auto& bin : pulse_vec) {
                charge += bin;
                charge_vec.push_back(charge);
            }

            name = "charge_ev" + std::to_string(event_num) + "_px" + std::to_string(pixel_index.x()) + "-" +
                   std::to_string(pixel_index.y());
            auto charge_graph = new TGraph(static_cast<int>(charge_vec.size()), &time[0], &charge_vec[0]);
            charge_graph->GetXaxis()->SetTitle("t [ns]");
            charge_graph->GetYaxis()->SetTitle("Q_{tot} [e]");
            charge_graph->SetTitle(("Accumulated induced charge in pixel (" + std::to_string(pixel_index.x()) + "," +
                                    std::to_string(pixel_index.y()) +
                                    "), Q_{tot} = " + std::to_string(pulse.getCharge()) + " e")
                                       .c_str());
            getROOTDirectory()->WriteTObject(charge_graph, name.c_str());

	    // -------- now the same for the amplified (and shaped) pulses
            std::vector<double> amptime(output_vec.size());
            // clang-format off
            std::generate(amptime.begin(), amptime.end(), [n = 0.0, timestep]() mutable {  auto now = n; n += timestep; return now; });
            // clang-format on

	    // asv there needs to be a better way to plot in mV ?
            std::vector<double> output_in_mV(output_vec.size());
	    double scaleConvert{1e9};
	    std::transform(output_vec.begin(), output_vec.end(), output_in_mV.begin(), [&scaleConvert](auto& c){return c*scaleConvert;});

            name = "output_ev" + std::to_string(event_num) + "_px" + std::to_string(pixel_index.x()) + "-" +
	           std::to_string(pixel_index.y());
            auto output_graph = new TGraph(static_cast<int>(output_vec.size()), &amptime[0], &output_in_mV[0]);
            output_graph->GetXaxis()->SetTitle("t [ns]");
            output_graph->GetYaxis()->SetTitle("CSA output [mV]");
            output_graph->SetTitle(("Amplifier signal in pixel (" + std::to_string(pixel_index.x()) + "," +
                                   std::to_string(pixel_index.y()) +  ")")
                                      .c_str());
            getROOTDirectory()->WriteTObject(output_graph, name.c_str());


	    // -------- now the same for the amplified (and shaped) pulses with noise
	    // asv there needs to be a better way to plot in mV ?
            std::vector<double> output_with_noise_in_mV(output_with_noise.size());
	    std::transform(output_with_noise.begin(), output_with_noise.end(), output_with_noise_in_mV.begin(), [&scaleConvert](auto& c){return c*scaleConvert;});

            name = "output_with_noise_ev" + std::to_string(event_num) + "_px" + std::to_string(pixel_index.x()) + "-" +
	           std::to_string(pixel_index.y());
            auto output_with_noise_graph = new TGraph(static_cast<int>(output_with_noise.size()), &amptime[0], &output_with_noise_in_mV[0]);
            output_with_noise_graph->GetXaxis()->SetTitle("t [ns]");
            output_with_noise_graph->GetYaxis()->SetTitle("CSA output [mV]");
            output_with_noise_graph->SetTitle(("Amplifier signal with added noise in pixel (" + std::to_string(pixel_index.x()) + "," +
                                   std::to_string(pixel_index.y()) +  ")")
                                      .c_str());
            getROOTDirectory()->WriteTObject(output_with_noise_graph, name.c_str());

	}


	// asv output "charge" of output pulse, or output tot? also add hit if never over threshold?
        // Add the hit to the hitmap
	//        hits.emplace_back(pixel, toa, output_pulse.getCharge(), &pixel_charge);
        hits.emplace_back(pixel, toa, tot, &pixel_charge);
    }

    // Output summary and update statistics
    LOG(INFO) << "Digitized " << hits.size() << " pixel hits";
    total_hits_ += hits.size();

    if(!hits.empty()) {
        // Create and dispatch hit message
        auto hits_message = std::make_shared<PixelHitMessage>(std::move(hits), getDetector());
        messenger_->dispatchMessage(this, hits_message);
    }
}

void CSADigitizerModule::finalize() {
    if(config_.get<bool>("output_plots")) {
        // Write histograms
        LOG(TRACE) << "Writing output plots to file";
        h_pxq->Write();
        h_tot->Write();
        h_toa->Write();
        h_pxq_vs_tot->Write();
	
    }

    LOG(INFO) << "Digitized " << total_hits_ << " pixel hits in total";
}


