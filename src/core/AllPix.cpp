/** @file
 *  @brief Implementation of interface to the core framework
 *  @copyright MIT License
 */

#include "AllPix.hpp"

#include <climits>
#include <memory>
#include <stdexcept>
#include <utility>

#include <TROOT.h>
#include <TRandom.h>
#include <TStyle.h>
#include <TSystem.h>

#include "core/config/exceptions.h"
#include "core/utils/file.h"
#include "core/utils/log.h"
#include "core/utils/random.h"
#include "core/utils/unit.h"

using namespace allpix;

/**
 * Initializes all the managers. This class will own the managers for the lifetime of the simulation.
 */
AllPix::AllPix(std::string file_name)
    : conf_mgr_(std::make_unique<ConfigManager>(std::move(file_name))), mod_mgr_(std::make_unique<ModuleManager>()),
      geo_mgr_(std::make_unique<GeometryManager>()), msg_(std::make_unique<Messenger>()) {}

/**
 * Performs the initialization, including:
 * - Configure the special header sections.
 * - Set the log level and log format as requested.
 * - Initialize the random seeder
 * - Determine and create the output directory
 * - Include all the defined units
 * - Load the modules from the configuration
 */
void AllPix::load() {
    // Configure the standard special sections
    conf_mgr_->setGlobalHeaderName("AllPix");
    conf_mgr_->addGlobalHeaderName("");
    conf_mgr_->addIgnoreHeaderName("Ignore");

    // Set the log level from config
    Configuration global_config = conf_mgr_->getGlobalConfiguration();
    std::string log_level_string = global_config.get<std::string>("log_level", "INFO");
    std::transform(log_level_string.begin(), log_level_string.end(), log_level_string.begin(), ::toupper);
    try {
        LogLevel log_level = Log::getLevelFromString(log_level_string);
        Log::setReportingLevel(log_level);
    } catch(std::invalid_argument& e) {
        throw InvalidValueError(global_config, "log_level", e.what());
    }

    // Set the log format from config
    std::string log_format_string = global_config.get<std::string>("log_format", "DEFAULT");
    std::transform(log_format_string.begin(), log_format_string.end(), log_format_string.begin(), ::toupper);
    try {
        LogFormat log_format = Log::getFormatFromString(log_format_string);
        Log::setFormat(log_format);
    } catch(std::invalid_argument& e) {
        throw InvalidValueError(global_config, "log_format", e.what());
    }

    // Wait for the debug messages until level and format are set
    LOG(DEBUG) << "Global log level is set to " << log_level_string;
    LOG(DEBUG) << "Global log format is set to " << log_format_string;

    // Initialize the random seeder
    if(global_config.has("random_seed")) {
        // Use provided random seed
        random_init(global_config.get<uint64_t>("random_seed"));
    } else {
        // Use entropy from the system
        random_init();
    }
    LOG(DEBUG) << "Initialized random seeder (first seed is " << get_random_seed() << ")";
    // Initialize ROOT random generator
    gRandom->SetSeed(get_random_seed());

    // Get output directory
    std::string directory = gSystem->pwd();
    directory += "/output";
    if(global_config.has("output_directory")) {
        // Use config specified one if available
        directory = global_config.getPath("output_directory");
    }
    // Set output directory (create if not exists)
    if(!gSystem->ChangeDirectory(directory.c_str())) {
        LOG(DEBUG) << "Creating output directory because it does not exists";
        try {
            allpix::create_directories(directory);
            gSystem->ChangeDirectory(directory.c_str());
        } catch(std::invalid_argument& e) {
            LOG(ERROR) << "Cannot create output directory " << directory << ": " << e.what()
                       << ". Using current directory instead!";
        }
    }

    // Set the default units to use
    add_units();

    // Set the ROOT style
    set_style();

    // Load the modules from the configuration
    LOG(DEBUG) << "Loading modules";
    mod_mgr_->load(msg_.get(), conf_mgr_.get(), geo_mgr_.get());
    LOG(DEBUG) << "Modules succesfully loaded"; // FIXME: add some more info
}

/**
 * Runs the Module::init() method linearly for every module
 */
void AllPix::init() {
    LOG(DEBUG) << "Initializing modules";
    mod_mgr_->init();
    LOG(DEBUG) << "Finished initialization";
}
/**
 * Runs every modules Module::run() method linearly for the number of events
 */
void AllPix::run() {
    Configuration global_config = conf_mgr_->getGlobalConfiguration();
    auto number_of_events = global_config.get<unsigned int>("number_of_events", 1u);
    LOG(DEBUG) << "Running modules for " << number_of_events << " events";
    mod_mgr_->run();
    LOG(DEBUG) << "Finished run";
}
/**
 * Runs all modules Module::finalize() method linearly for every module
 */
void AllPix::finalize() {
    LOG(DEBUG) << "Finalizing modules";
    mod_mgr_->finalize();
    LOG(DEBUG) << "Finalization completed";
}

void AllPix::add_units() {
    // LENGTH
    Units::add("nm", 1e-6);
    Units::add("um", 1e-3);
    Units::add("mm", 1);
    Units::add("cm", 1e1);
    Units::add("dm", 1e2);
    Units::add("m", 1e3);
    Units::add("km", 1e6);

    // TIME
    Units::add("ps", 1e-3);
    Units::add("ns", 1);
    Units::add("us", 1e3);
    Units::add("ms", 1e6);
    Units::add("s", 1e9);

    // TEMPERATURE
    Units::add("K", 1);

    // ENERGY
    Units::add("eV", 1e-6);
    Units::add("keV", 1e-3);
    Units::add("MeV", 1);
    Units::add("GeV", 1e3);

    // CHARGE
    Units::add("e", 1);
    Units::add("C", 1.6021766208e-19);

    // VOLTAGE
    // NOTE: fixed by above
    Units::add("V", 1e-6);
    Units::add("kV", 1e-3);

    // ANGLES
    // NOTE: these are fake units
    Units::add("deg", 0.01745329252);
    Units::add("rad", 1);
}

/**
 * This style is inspired by the CLICdp plot style
 */
void AllPix::set_style() {
    // use plain style as base
    gROOT->SetStyle("Plain");
    TStyle* style = gROOT->GetStyle("Plain");

    // set backgrounds
    style->SetCanvasColor(kWhite);
    style->SetFrameFillColor(kWhite);
    style->SetStatColor(kWhite);
    style->SetPadColor(kWhite);
    style->SetFillColor(10);
    style->SetTitleFillColor(kWhite);

    // SetPaperSize wants width & height in cm: A4 is 20,26
    style->SetPaperSize(20, 26);
    // No yellow border around histogram
    style->SetDrawBorder(0);
    // remove border of canvas*
    style->SetCanvasBorderMode(0);
    // remove border of pads
    style->SetPadBorderMode(0);
    style->SetFrameBorderMode(0);
    style->SetLegendBorderSize(0);

    // default text size
    style->SetTextSize(0.04f);
    style->SetTitleSize(0.04f, "xyz");
    style->SetLabelSize(0.03f, "xyz");

    // title offset: distance between given text and axis
    style->SetLabelOffset(0.01f, "xyz");
    style->SetTitleOffset(1.6f, "yz");
    style->SetTitleOffset(1.4f, "x");

    // set font settings
    short font = 42; // use a clear font
    style->SetTitleFont(font);
    style->SetTitleFontSize(0.06f);
    style->SetStatFont(font);
    style->SetStatFontSize(0.07f);
    style->SetTextFont(font);
    style->SetLabelFont(font, "xyz");
    style->SetTitleFont(font, "xyz");
    style->SetTitleBorderSize(0);
    style->SetStatBorderSize(1);

    // set style for markers
    style->SetMarkerStyle(1);
    style->SetLineWidth(2);
    style->SetMarkerSize(1.2f);

    // set palette in 2d histogram to nice and colorful one
    style->SetPalette(1, 0);

    // disable title by default for histograms
    style->SetOptTitle(0);

    // set statistics
    style->SetOptStat(0);
    style->SetOptFit(0);

    // number of decimals used for errors
    style->SetEndErrorSize(5);

    // set line width to 2 by default so that histograms are visible when printed small
    // idea: emphasize the data, not the frame around
    style->SetHistLineWidth(2);
    style->SetFrameLineWidth(2);
    style->SetFuncWidth(2);
    style->SetHistLineColor(kBlack);
    style->SetFuncColor(kRed);
    style->SetLabelColor(kBlack, "xyz");

    // set the margins
    style->SetPadBottomMargin(0.18f);
    style->SetPadTopMargin(0.08f);
    style->SetPadRightMargin(0.18f);
    style->SetPadLeftMargin(0.17f);

    // set the default number of divisions to show
    style->SetNdivisions(506, "xy");

    // turn off xy grids
    style->SetPadGridX(0);
    style->SetPadGridY(0);

    // set the tick mark style
    style->SetPadTickX(1);
    style->SetPadTickY(1);
    style->SetCanvasDefW(800);
    style->SetCanvasDefH(700);

    // force the style
    gROOT->ForceStyle();
}
