/**
 * @file
 * @brief Implementation of interface to the core framework
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "Allpix.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <climits>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <ios>
#include <memory>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <TROOT.h>
#include <TStyle.h>
#include <TSystem.h>

#include "core/config/ConfigManager.hpp"
#include "core/config/FileParser.hpp"
#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/ModuleManager.hpp"
#include "core/utils/log.h"
#include "tools/units.h"

using namespace allpix;

/**
 * This class will own the managers for the lifetime of the simulation. Will do early initialization:
 * - Configure the special header sections.
 * - Set the log level and log format as requested.
 * - Load the detector configuration and parse it
 */
Allpix::Allpix(std::filesystem::path config_file_name,
               const std::vector<std::string>& module_options,
               const std::vector<std::string>& detector_options)
    : has_run_(false), msg_(std::make_unique<Messenger>()), geo_mgr_(std::make_unique<GeometryManager>()),
      mod_mgr_(std::make_unique<ModuleManager>()) {

    // Load the configuration from file
    load_configuration(std::move(config_file_name), module_options, detector_options);

    // Fetch the global configuration
    const auto& global_config = conf_mgr_->getGlobalConfiguration();

    // Set the log level from config if not specified earlier
    std::string log_level_string;
    if(Log::getReportingLevel() == LogLevel::NONE) {
        log_level_string = global_config.get<std::string>("log_level", "WARNING");
        std::transform(log_level_string.begin(), log_level_string.end(), log_level_string.begin(), ::toupper);
        try {
            log_level_ = Log::getLevelFromString(log_level_string);
        } catch(std::invalid_argument& e) {
            LOG(ERROR) << "Log level \"" << log_level_string
                       << "\" specified in the configuration is invalid, defaulting to WARNING instead";
            log_level_ = LogLevel::WARNING;
        }
    }
    Log::setReportingLevel(log_level_);

    // Set the log format from config
    auto log_format_string = global_config.get<std::string>("log_format", "DEFAULT");
    std::transform(log_format_string.begin(), log_format_string.end(), log_format_string.begin(), ::toupper);
    try {
        LogFormat const log_format = Log::getFormatFromString(log_format_string);
        Log::setFormat(log_format);
    } catch(std::invalid_argument& e) {
        LOG(ERROR) << "Log format \"" << log_format_string
                   << "\" specified in the configuration is invalid, using DEFAULT instead";
        Log::setFormat(LogFormat::DEFAULT);
    }

    // Open log file to write output to
    if(global_config.has("log_file")) {
        // NOTE: this stream should be available for the duration of the logging
        log_file_.open(global_config.getPath("log_file"), std::ios_base::out | std::ios_base::trunc);
        LOG(TRACE) << "Added log stream to file " << global_config.getPath("log_file");
        Log::addStream(log_file_);
    }

    // Wait for the first detailed messages until level and format are properly set
    LOG(TRACE) << "Global log level is set to " << Log::getStringFromLevel(Log::getReportingLevel());
    LOG(TRACE) << "Global log format is set to " << log_format_string;
}

void Allpix::load_configuration(std::filesystem::path config_file_name,
                                const std::vector<std::string>& module_options,
                                const std::vector<std::string>& detector_options) {

    // Check if the configuration file exists
    std::ifstream file(config_file_name);
    if(!file || !std::filesystem::is_regular_file(config_file_name)) {
        throw ConfigFileUnavailableError(config_file_name);
    }

    // Convert main file to absolute path and read the file
    LOG(TRACE) << "Reading main configuration";
    config_file_name = std::filesystem::canonical(config_file_name);
    const auto stack_cfg = FileParser::getStack(file, std::move(config_file_name));

    // Load the global configuration
    conf_mgr_ = std::make_unique<ConfigManager>(stack_cfg.getHeaderConfiguration(),
                                                stack_cfg.getConfigurations(),
                                                std::initializer_list<std::string>({"Allpix", ""}),
                                                std::initializer_list<std::string>({"Ignore"}));

    // Load and apply the provided module options
    conf_mgr_->loadModuleOptions(module_options);

    // Read the detector file and load it to the config manager
    auto detector_file_name = conf_mgr_->getGlobalConfiguration().getPath("detectors_file", true);
    LOG(TRACE) << "Reading detector configuration";
    std::ifstream detector_file(detector_file_name);
    const auto stack_geo = FileParser::getStack(detector_file, std::move(detector_file_name));
    conf_mgr_->loadDetectors(stack_geo.getConfigurations());

    // Load and apply the provided detector options
    conf_mgr_->loadDetectorOptions(detector_options);
}

/**
 * Performs the initialization, including:
 * - Initialize the random seeder
 * - Determine and create the output directory
 * - Include all the defined units
 * - Load the modules from the configuration
 */
void Allpix::load() {
    Log::setReportingLevel(log_level_);
    if(stop_source_.get_token().stop_requested()) {
        LOG(INFO) << "Skip loading modules because termination is requested";
        return;
    }

    LOG(TRACE) << "Loading Allpix";

    // Fetch the global configuration
    Configuration& global_config = conf_mgr_->getGlobalConfiguration();

    // Put welcome message and set version
    LOG(STATUS) << "Welcome to Allpix^2 " << ALLPIX_PROJECT_VERSION;
    global_config.set<std::string>("version", ALLPIX_PROJECT_VERSION, true);

    uint64_t seed = 0;
    if(global_config.has("random_seed")) {
        // Use provided random seed
        seed = global_config.get<uint64_t>("random_seed");
        seeder_modules_.seed(seed);
        LOG(STATUS) << "Initialized PRNG with configured seed " << seed;
    } else {
        // Compute random entropy seed
        // Use the clock
        auto clock_seed = static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        // Use memory location local variable
        auto mem_seed = reinterpret_cast<uint64_t>(&seed); // NOLINT
        // Use thread id
        std::hash<std::thread::id> const thrd_hasher;
        auto thread_seed = thrd_hasher(std::this_thread::get_id());
        seed = (clock_seed ^ mem_seed ^ thread_seed);
        seeder_modules_.seed(seed);
        LOG(STATUS) << "Initialized PRNG with system entropy seed " << seed;
        global_config.set<uint64_t>("random_seed", seed, true);
    }

    if(global_config.has("random_seed_core")) {
        // Use provided random seed
        seed = global_config.get<uint64_t>("random_seed_core");
        seeder_core_.seed(seed);
        LOG(STATUS) << "Initialized core PRNG with configured seed " << seed;
    } else {
        // Use module seeder + 1
        seeder_core_.seed(seed + 1);
        global_config.set<uint64_t>("random_seed_core", seed + 1, true);
    }

    // Get output directory
    std::string directory = gSystem->pwd();
    directory += "/output";
    if(global_config.has("output_directory")) {
        // Use config specified one if available
        directory = global_config.getPath("output_directory");
    }

    // Use existing output directory if it exists
    bool create_output_dir = true;
    if(std::filesystem::is_directory(directory)) {
        if(global_config.get<bool>("purge_output_directory", false)) {
            LOG(DEBUG) << "Deleting previous output directory " << directory;
            std::filesystem::remove_all(directory);
        } else {
            LOG(DEBUG) << "Output directory " << directory << " already exists";
            create_output_dir = false;
        }
    }
    // Create the output directory
    try {
        if(create_output_dir) {
            LOG(DEBUG) << "Creating output directory " << directory;
            std::filesystem::create_directories(directory);
        }
        // Change to the new/existing output directory
        gSystem->ChangeDirectory(directory.c_str());
    } catch(std::invalid_argument& e) {
        LOG(ERROR) << "Cannot create output directory " << directory << ": " << e.what()
                   << ". Using current directory instead.";
    }

    // Enable relevant multithreading safety in ROOT
    // Required for spawned threads, even with a single worker
    ROOT::EnableThreadSafety();

    // Set the default units to use
    register_units();

    // Set the ROOT style
    set_style();

    // Load the geometry
    load_geometry();

    // Get a histogram manager
    histo_mgr_ = std::make_unique<HistogramManager>(global_config.get<std::string>("root_file", "histograms"),
                                                    global_config.get<bool>("deny_overwrite", false));

    // Load the modules from the configuration
    mod_mgr_->load(msg_.get(), conf_mgr_.get(), geo_mgr_.get(), histo_mgr_.get());
}

void Allpix::load_geometry() {

    // Fetch the global configuration
    const auto& global_config = conf_mgr_->getGlobalConfiguration();

    geo_mgr_->loadGeometry(conf_mgr_->getDetectorConfigurations(), seeder_core_);

    // Load model files:
    LOG(TRACE) << "Loading remaining default models";

    // Get paths to read models from
    LOG(TRACE) << "Reading model files";
    const auto model_paths = get_model_paths(global_config.getFilePath());

    // Add all the paths to the reader
    for(const auto& path : model_paths) {
        // Check if file or directory
        if(std::filesystem::is_directory(path)) {
            for(const auto& entry : std::filesystem::directory_iterator(path)) {
                if(!entry.is_regular_file()) {
                    continue;
                }

                // Accept only with correct model suffix
                auto sub_path = std::filesystem::canonical(entry);
                std::string const suffix(ALLPIX_MODEL_SUFFIX);
                if(sub_path.extension() != suffix) {
                    continue;
                }

                // Read model file and add model to list
                read_model_file(sub_path);
            }
        } else {
            // Always a file because paths are already checked
            read_model_file(path);
        }
    }
}

std::vector<std::filesystem::path> Allpix::get_model_paths(const std::filesystem::path& config_file_path) {

    std::vector<std::filesystem::path> model_paths{};

    // Fetch the global configuration
    const auto& global_config = conf_mgr_->getGlobalConfiguration();

    // Load the list of standard model paths
    if(global_config.has("model_paths")) {
        model_paths = global_config.getPathArray("model_paths", true);
    }

    if(std::filesystem::is_directory(ALLPIX_MODEL_DIRECTORY)) {
        model_paths.emplace_back(ALLPIX_MODEL_DIRECTORY);
        LOG(TRACE) << "Registered model path: " << ALLPIX_MODEL_DIRECTORY;
    }
    const char* data_dirs_env = std::getenv("XDG_DATA_DIRS"); // NOLINT(concurrency-mt-unsafe)
    if(data_dirs_env == nullptr || strlen(data_dirs_env) == 0) {
        data_dirs_env = "/usr/local/share/:/usr/share/:";
    }
    auto data_dirs = split<std::filesystem::path>(data_dirs_env, ":");
    for(auto data_dir : data_dirs) {
        data_dir /= std::filesystem::path(ALLPIX_PROJECT_NAME) / std::string("models");
        if(std::filesystem::is_directory(data_dir)) {
            model_paths.emplace_back(data_dir);
            LOG(TRACE) << "Registered global model path: " << data_dir;
        }
    }
    if(!config_file_path.empty() && std::filesystem::is_directory(config_file_path.parent_path())) {
        model_paths.emplace_back(config_file_path.parent_path());
        LOG(TRACE) << "Registered path of configuration file as model location.";
    }

    return model_paths;
}

void Allpix::read_model_file(const std::filesystem::path& path) {
    auto model_name = path.stem();
    LOG(TRACE) << "Reading model " << model_name << " in path " << path;

    try {
        // Try to parse as config file
        std::ifstream file(path);
        const auto stack = FileParser::getStack(file, path);

        // Prefer in-file name field over file name:
        model_name = stack.getHeaderConfiguration().get<std::string>("name", model_name);

        // Check if we need to look at file at all
        if(geo_mgr_->hasModel(model_name)) {
            LOG(DEBUG) << "Skipping overwritten model " << model_name << " in path " << path;
            return;
        }
        if(!geo_mgr_->needsModel(model_name)) {
            LOG(TRACE) << "Skipping not required model " << model_name << " in path " << path;
            return;
        }

        // Parse configuration and add model to the config
        geo_mgr_->addModel(DetectorModel::factory(model_name, stack));

    } catch(const ConfigParseError& e) {
        // Not a valid config file, see https://gitlab.cern.ch/allpix-squared/allpix-squared/-/issues/277
        LOG(ERROR) << "Skipping invalid model file \"" << path << "\":" << '\n' << e.what();
    }
}

/**
 * Runs the Module::initialize() method linearly for every module
 */
void Allpix::initialize() {
    Log::setReportingLevel(log_level_);
    if(stop_source_.get_token().stop_requested()) {
        LOG(INFO) << "Skip initializing modules because termination is requested";
        return;
    }

    LOG(TRACE) << "Initializing Allpix";
    mod_mgr_->initialize();
}

/**
 * Start the thread which calls the ModuleManager's run method
 */
void Allpix::start() { run_thread_ = std::jthread(std::bind_front(&Allpix::run, this)); }

/**
 * Check for any exception thrown in the run thread
 */
void Allpix::checkException() {
    // If exception has been thrown, propagate it
    if(exception_ptr_) {
        const auto exception_ptr_copy = exception_ptr_;
        exception_ptr_ = nullptr;
        std::rethrow_exception(exception_ptr_copy);
    }
}

/**
 * Runs every modules Module::run() method linearly for the number of events
 */
void Allpix::run(const std::stop_token& /*unused*/) {

    try {
        Log::setReportingLevel(log_level_);
        const auto& stop_token = stop_source_.get_token();
        if(stop_token.stop_requested()) {
            LOG(INFO) << "Skip running modules because termination is requested";
            return;
        }

        LOG(TRACE) << "Running Allpix";
        mod_mgr_->run(seeder_modules_, stop_token);
    } catch(...) {
        LOG(ERROR) << "Caught exception in run thread";
        exception_ptr_ = std::current_exception();
    }

    // Indicate that we have run and want to finalize as well
    has_run_ = true;
}

void Allpix::interrupt() { stop_source_.request_stop(); }

void Allpix::wait() {
    if(run_thread_.joinable()) {
        run_thread_.join();
    }
}

/**
 * Runs all modules Module::finalize() method linearly for every module
 */
void Allpix::finalize() {
    Log::setReportingLevel(log_level_);
    LOG(TRACE) << "Finalizing Allpix";

    if(has_run_) {
        mod_mgr_->finalize();
    } else {
        LOG(INFO) << "Skip finalizing modules because no module did run";
    }

    histo_mgr_->finalize();
}

/**
 * This style is inspired by the CLICdp plot style
 */
void Allpix::set_style() {
    LOG(TRACE) << "Setting ROOT plotting style";

    // use plain style as base
    gROOT->SetStyle("Plain");
    TStyle* style = gROOT->GetStyle("Plain");

    // Prefer OpenGL if available
    style->SetCanvasPreferGL(kTRUE);

    // Set backgrounds
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
    // Remove border of canvas*
    style->SetCanvasBorderMode(0);
    // Remove border of pads
    style->SetPadBorderMode(0);
    style->SetFrameBorderMode(0);
    style->SetLegendBorderSize(0);

    // Default text size
    style->SetTextSize(0.04F);
    style->SetTitleSize(0.04F, "xyz");
    style->SetLabelSize(0.03F, "xyz");

    // Title offset: distance between given text and axis
    style->SetLabelOffset(0.01F, "xyz");
    style->SetTitleOffset(1.4F, "yz");
    style->SetTitleOffset(1.4F, "x");

    // Set font settings
    short const font = 42; // Use a clear font
    style->SetTitleFont(font);
    style->SetTitleFontSize(0.06F);
    style->SetStatFont(font);
    style->SetStatFontSize(0.07F);
    style->SetTextFont(font);
    style->SetLabelFont(font, "xyz");
    style->SetTitleFont(font, "xyz");
    style->SetTitleBorderSize(0);
    style->SetStatBorderSize(1);

    // Set style for markers
    style->SetMarkerStyle(1);
    style->SetLineWidth(2);
    style->SetMarkerSize(1.2F);

    // Set palette in 2d histogram to nice and colorful one
    style->SetPalette(1, nullptr);

    // Disable title by default for histograms
    style->SetOptTitle(0);

    // Set statistics
    style->SetOptStat(0);
    style->SetOptFit(0);

    // Number of decimals used for errors
    style->SetEndErrorSize(5);

    // Set line width to 2 by default so that histograms are visible when printed small
    // Idea: emphasize the data, not the frame around
    style->SetHistLineWidth(2);
    style->SetFrameLineWidth(2);
    style->SetFuncWidth(2);
    style->SetHistLineColor(kBlack);
    style->SetFuncColor(kRed);
    style->SetLabelColor(kBlack, "xyz");

    // Set the margins
    style->SetPadBottomMargin(0.18F);
    style->SetPadTopMargin(0.08F);
    style->SetPadRightMargin(0.18F);
    style->SetPadLeftMargin(0.17F);

    // Set the default number of divisions to show
    style->SetNdivisions(506, "xy");

    // Turn off xy grids
    style->SetPadGridX(false);
    style->SetPadGridY(false);

    // Set the tick mark style
    style->SetPadTickX(1);
    style->SetPadTickY(1);
    style->SetCanvasDefW(800);
    style->SetCanvasDefH(700);

    // Force the style
    gROOT->ForceStyle();
}
