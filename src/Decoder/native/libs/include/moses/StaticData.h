// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#ifndef moses_StaticData_h
#define moses_StaticData_h

#include <stdexcept>
#include <limits>
#include <list>
#include <vector>
#include <map>
#include <memory>
#include <utility>
#include <fstream>
#include <string>

#ifdef WITH_THREADS
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#endif

#include "Parameter.h"
#include "SentenceStats.h"
#include "ScoreComponentCollection.h"
#include "moses/FF/Factory.h"
#include "moses/PP/Factory.h"

#include "moses/parameters/AllOptions.h"
#include "moses/parameters/BookkeepingOptions.h"

namespace Moses
{

class InputType;
class DecodeGraph;
class DecodeStep;

class DynamicCacheBasedLanguageModel;
class PhraseDictionaryDynamicCacheBased;

typedef std::pair<std::string, float> UnknownLHSEntry;
typedef std::vector<UnknownLHSEntry>  UnknownLHSList;

/** Contains global variables and contants.
 *  Only 1 object of this class should be instantiated.
 *  A const object of this class is accessible by any function during decoding by calling StaticData::Instance();
 */
class StaticData
{
  friend class HyperParameterAsWeight;

private:
  static StaticData s_instance;

  std::map<pthread_t, ttasksptr> m_ttasks;
#ifdef WITH_THREADS
  boost::mutex m_ttasks_lock;
#endif

protected:
  Parameter *m_parameter;
  AllOptions m_options;

  mutable ScoreComponentCollection m_allWeights;

  std::vector<DecodeGraph*> m_decodeGraphs;

  // Initial	= 0 = can be used when creating poss trans
  // Other		= 1 = used to calculate LM score once all steps have been processed
  float
  m_wordDeletionWeight;


  // PhraseTrans, Generation & LanguageModelScore has multiple weights.
  // int				m_maxDistortion;
  // do it differently from old pharaoh
  // -ve	= no limit on distortion
  // 0		= no disortion (monotone in old pharaoh)
  bool m_reorderingConstraint; //! use additional reordering constraints
  BookkeepingOptions m_bookkeeping_options;

  size_t m_latticeSamplesSize;

  std::string  m_latticeSamplesFilePath;
  // bool m_dropUnknown; //! false = treat unknown words as unknowns, and translate them as themselves; true = drop (ignore) them
  // bool m_markUnknown; //! false = treat unknown words as unknowns, and translate them as themselves; true = mark and (ignore) them
  // std::string m_unknownWordPrefix;
  // std::string m_unknownWordSuffix;
  bool m_wordDeletionEnabled;

  bool m_disableDiscarding;
  bool m_printAllDerivations;
  bool m_printTranslationOptions;

  bool m_sourceStartPosMattersForRecombination;
  bool m_requireSortingAfterSourceContext;

  mutable size_t m_verboseLevel;

  std::string m_factorDelimiter; //! by default, |, but it can be changed

  // XmlInputType m_xmlInputType; //! method for handling sentence XML input
  std::pair<std::string,std::string> m_xmlBrackets; //! strings to use as XML tags' opening and closing brackets. Default are "<" and ">"

  size_t m_lmcache_cleanup_threshold; //! number of translations after which LM claenup is performed (0=never, N=after N translations; default is 1)
  bool m_lmEnableOOVFeature;

  bool m_isAlwaysCreateDirectTranslationOption;
  //! constructor. only the 1 static variable can be created

  bool m_includeLHSInSearchGraph; //! include LHS of rules in search graph
  std::string m_outputUnknownsFile; //! output unknowns in this file

  size_t m_ruleLimit;

  // Whether to load compact phrase table and reordering table into memory
  bool m_minphrMemory;
  bool m_minlexrMemory;

  // Initial = 0 = can be used when creating poss trans
  // Other = 1 = used to calculate LM score once all steps have been processed
  Word m_inputDefaultNonTerminal, m_outputDefaultNonTerminal;
  SourceLabelOverlap m_sourceLabelOverlap;
  UnknownLHSList m_unknownLHS;

  int m_threadCount;
  long m_startTranslationId;

  // alternate weight settings
  mutable std::string m_currentWeightSetting;
  std::map< std::string, ScoreComponentCollection* > m_weightSetting; // core weights
  std::map< std::string, std::set< std::string > > m_weightSettingIgnoreFF; // feature function
  std::map< std::string, std::set< size_t > > m_weightSettingIgnoreDP; // decoding path

  // FactorType m_placeHolderFactor;
  bool m_useLegacyPT;
  bool m_defaultNonTermOnlyForEmptyRange;
  S2TParsingAlgorithm m_s2tParsingAlgorithm;

  FeatureRegistry m_registry;
  PhrasePropertyFactory m_phrasePropertyFactory;

  StaticData();

  void LoadChartDecodingParameters();
  void LoadNonTerminals();

  //! load decoding steps
  void LoadDecodeGraphs();
  void LoadDecodeGraphsOld(const std::vector<std::string> &mappingVector,
                           const std::vector<size_t> &maxChartSpans);
  void LoadDecodeGraphsNew(const std::vector<std::string> &mappingVector,
                           const std::vector<size_t> &maxChartSpans);

  void NoCache();

  bool m_continuePartialTranslation;
  std::string m_binPath;

  // soft NT lookup for chart models
  std::vector<std::vector<Word> > m_softMatchesMap;

  const StatefulFeatureFunction* m_treeStructure;

  void ini_compact_table_options();
  void ini_consensus_decoding_options();
  void ini_cube_pruning_options();
  void ini_distortion_options();
  void ini_factor_maps();
  void ini_input_options();
  void ini_lm_options();
  void ini_lmbr_options();
  void ini_mbr_options();
  void ini_mira_options();
  void ini_oov_options();
  bool ini_output_options();
  bool ini_performance_options();
  void ini_phrase_lookup_options();
  bool ini_stack_decoding_options();
  void ini_zombie_options();

  void initialize_features();
public:

  bool IsAlwaysCreateDirectTranslationOption() const {
    return m_isAlwaysCreateDirectTranslationOption;
  }
  //! destructor
  ~StaticData();

  //! return static instance for use like global variable
  static const StaticData& Instance() {
    return s_instance;
  }

  //! do NOT call unless you know what you're doing
  static StaticData& InstanceNonConst() {
    return s_instance;
  }

  /** delete current static instance and replace with another.
  	* Used by gui front end
  	*/
#ifdef WIN32
  static void Reset() {
    s_instance = StaticData();
  }
#endif

  //! Load data into static instance. This function is required as
  //  LoadData() is not const
  static bool LoadDataStatic(Parameter *parameter, const std::string &execPath);

  //! Main function to load everything. Also initialize the Parameter object
  bool LoadData(Parameter *parameter);
  void ClearData();

  const Parameter &GetParameter() const {
    return *m_parameter;
  }

  AllOptions const&
  options() const {
    return m_options;
  }

  AllOptions&
  options() {
    return m_options;
  }

  const std::vector<FactorType> &GetInputFactorOrder() const {
    return m_options.input.factor_order;
  }

  const std::vector<FactorType> &GetOutputFactorOrder() const {
    return m_options.output.factor_order;
  }

  inline bool GetSourceStartPosMattersForRecombination() const {
    return m_sourceStartPosMattersForRecombination;
  }
  // inline bool GetDropUnknown() const {
  //   return m_dropUnknown;
  // }
  // inline bool GetMarkUnknown() const {
  //   return m_markUnknown;
  // }
  // inline std::string GetUnknownWordPrefix() const {
  //   return m_unknownWordPrefix;
  // }
  // inline std::string GetUnknownWordSuffix() const {
  //   return m_unknownWordSuffix;
  // }
  inline bool GetDisableDiscarding() const {
    return m_disableDiscarding;
  }
  inline size_t GetMaxNoTransOptPerCoverage() const {
    return m_options.search.max_trans_opt_per_cov;
  }
  inline size_t GetMaxNoPartTransOpt() const {
    return m_options.search.max_partial_trans_opt;
  }
  inline size_t GetMaxPhraseLength() const {
    return m_options.search.max_phrase_length;
  }
  bool IsWordDeletionEnabled() const {
    return m_wordDeletionEnabled;
  }

  int GetMaxDistortion() const {
    return m_options.reordering.max_distortion;
  }
  bool UseReorderingConstraint() const {
    return m_reorderingConstraint;
  }

  bool UseEarlyDiscarding() const {
    return m_options.search.early_discarding_threshold
           != -std::numeric_limits<float>::infinity();
  }
  bool UseEarlyDistortionCost() const {
    return m_options.reordering.use_early_distortion_cost;
    // return m_useEarlyDistortionCost;
  }
  float GetTranslationOptionThreshold() const {
    return m_options.search.trans_opt_threshold;
  }

  size_t GetVerboseLevel() const {
    return m_verboseLevel;
  }
  void SetVerboseLevel(int x) const {
    m_verboseLevel = x;
  }

  bool UseMinphrInMemory() const {
    return m_minphrMemory;
  }

  bool UseMinlexrInMemory() const {
    return m_minlexrMemory;
  }

  size_t GetLatticeSamplesSize() const {
    return m_latticeSamplesSize;
  }

  const std::string& GetLatticeSamplesFilePath() const {
    return m_latticeSamplesFilePath;
  }

  bool IsSyntax(SearchAlgorithm algo = DefaultSearchAlgorithm) const {
    if (algo == DefaultSearchAlgorithm)
      algo = m_options.search.algo;

    return (algo == CYKPlus   || algo == ChartIncremental ||
            algo == SyntaxS2T || algo == SyntaxT2S ||
            algo == SyntaxF2S || algo == SyntaxT2S_SCFG);
  }

  const ScoreComponentCollection&
  GetAllWeights() const {
    return m_allWeights;
  }

  void SetAllWeights(const ScoreComponentCollection& weights) {
    m_allWeights = weights;
  }

  //Weight for a single-valued feature
  float GetWeight(const FeatureFunction* sp) const {
    return m_allWeights.GetScoreForProducer(sp);
  }

  //Weight for a single-valued feature
  void SetWeight(const FeatureFunction* sp, float weight) ;


  //Weights for feature with fixed number of values
  std::vector<float> GetWeights(const FeatureFunction* sp) const {
    return m_allWeights.GetScoresForProducer(sp);
  }

  //Weights for feature with fixed number of values
  void SetWeights(const FeatureFunction* sp, const std::vector<float>& weights);

  const std::string& GetFactorDelimiter() const {
    return m_factorDelimiter;
  }

  size_t GetLMCacheCleanupThreshold() const {
    return m_lmcache_cleanup_threshold;
  }

  bool GetLMEnableOOVFeature() const {
    return m_lmEnableOOVFeature;
  }

  const std::string& GetOutputUnknownsFile() const {
    return m_outputUnknownsFile;
  }

  bool GetIncludeLHSInSearchGraph() const {
    return m_includeLHSInSearchGraph;
  }

  // XmlInputType GetXmlInputType() const {
  //   return m_xmlInputType;
  // }

  std::pair<std::string,std::string> GetXmlBrackets() const {
    return m_xmlBrackets;
  }

  bool PrintTranslationOptions() const {
    return m_printTranslationOptions;
  }

  bool PrintAllDerivations() const {
    return m_printAllDerivations;
  }

  const UnknownLHSList &GetUnknownLHS() const {
    return m_unknownLHS;
  }

  const Word &GetInputDefaultNonTerminal() const {
    return m_inputDefaultNonTerminal;
  }
  const Word &GetOutputDefaultNonTerminal() const {
    return m_outputDefaultNonTerminal;
  }

  SourceLabelOverlap GetSourceLabelOverlap() const {
    return m_sourceLabelOverlap;
  }

  size_t GetRuleLimit() const {
    return m_ruleLimit;
  }
  float GetRuleCountThreshold() const {
    return 999999; /* TODO wtf! */
  }

  bool ContinuePartialTranslation() const {
    return m_continuePartialTranslation;
  }

  void ReLoadBleuScoreFeatureParameter(float weight);

  Parameter* GetParameter() {
    return m_parameter;
  }

  int ThreadCount() const {
    return m_threadCount;
  }

  long GetStartTranslationId() const {
    return m_startTranslationId;
  }

  void SetExecPath(const std::string &path);
  const std::string &GetBinDirectory() const;

  bool NeedAlignmentInfo() const {
    return m_bookkeeping_options.need_alignment_info;
  }

  bool GetHasAlternateWeightSettings() const {
    return m_weightSetting.size() > 0;
  }

  /** Alternate weight settings allow the wholesale ignoring of
      feature functions. This function checks if a feature function
      should be evaluated given the current weight setting */
  bool IsFeatureFunctionIgnored( const FeatureFunction &ff ) const {
    if (!GetHasAlternateWeightSettings()) {
      return false;
    }
    std::map< std::string, std::set< std::string > >::const_iterator lookupIgnoreFF
    =  m_weightSettingIgnoreFF.find( m_currentWeightSetting );
    if (lookupIgnoreFF == m_weightSettingIgnoreFF.end()) {
      return false;
    }
    const std::string &ffName = ff.GetScoreProducerDescription();
    const std::set< std::string > &ignoreFF = lookupIgnoreFF->second;
    return ignoreFF.count( ffName );
  }

  /** Alternate weight settings allow the wholesale ignoring of
      decoding graphs (typically a translation table). This function
      checks if a feature function should be evaluated given the
      current weight setting */
  bool IsDecodingGraphIgnored( const size_t id ) const {
    if (!GetHasAlternateWeightSettings()) {
      return false;
    }
    std::map< std::string, std::set< size_t > >::const_iterator lookupIgnoreDP
    =  m_weightSettingIgnoreDP.find( m_currentWeightSetting );
    if (lookupIgnoreDP == m_weightSettingIgnoreDP.end()) {
      return false;
    }
    const std::set< size_t > &ignoreDP = lookupIgnoreDP->second;
    return ignoreDP.count( id );
  }

  /** process alternate weight settings
    * (specified with [alternate-weight-setting] in config file) */
  void SetWeightSetting(const std::string &settingName) const {

    // if no change in weight setting, do nothing
    if (m_currentWeightSetting == settingName) {
      return;
    }

    // model must support alternate weight settings
    if (!GetHasAlternateWeightSettings()) {
      std::cerr << "Warning: Input specifies weight setting, but model does not support alternate weight settings.";
      return;
    }

    // find the setting
    m_currentWeightSetting = settingName;
    std::map< std::string, ScoreComponentCollection* >::const_iterator i =
      m_weightSetting.find( settingName );

    // if not found, resort to default
    if (i == m_weightSetting.end()) {
      std::cerr << "Warning: Specified weight setting " << settingName
                << " does not exist in model, using default weight setting instead";
      i = m_weightSetting.find( "default" );
      m_currentWeightSetting = "default";
    }

    // set weights
    m_allWeights = *(i->second);
  }

  float GetWeightWordPenalty() const;

  const std::vector<DecodeGraph*>& GetDecodeGraphs() const {
    return m_decodeGraphs;
  }

  //sentence (and thread) specific initialisationn and cleanup
  // void InitializeForInput(const InputType& source, ttaskptr const& ttask) const;
  void InitializeForInput(ttasksptr const& ttask) const;
  void CleanUpAfterSentenceProcessing(ttasksptr const& ttask) const;

  void LoadFeatureFunctions();
  bool CheckWeights() const;
  void LoadSparseWeightsFromConfig();
  bool LoadWeightSettings();
  bool LoadAlternateWeightSettings();

  std::map<std::string, std::string> OverrideFeatureNames();
  void OverrideFeatures();

  // FactorType GetPlaceholderFactor() const {
  //   return m_placeHolderFactor;
  // }

  const FeatureRegistry &GetFeatureRegistry() const {
    return m_registry;
  }

  const PhrasePropertyFactory &GetPhrasePropertyFactory() const {
    return m_phrasePropertyFactory;
  }

  /** check whether we should be using the old code to support binary phrase-table.
  ** eventually, we'll stop support the binary phrase-table and delete this legacy code
  **/
  void CheckLEGACYPT();
  bool GetUseLegacyPT() const {
    return m_useLegacyPT;
  }

  void SetSoftMatches(std::vector<std::vector<Word> >& softMatchesMap) {
    m_softMatchesMap = softMatchesMap;
  }

  const std::vector< std::vector<Word> >& GetSoftMatches() const {
    return m_softMatchesMap;
  }

  void ResetWeights(const std::string &denseWeights, const std::string &sparseFile);

  // need global access for output of tree structure
  const StatefulFeatureFunction* GetTreeStructure() const {
    return m_treeStructure;
  }

  void SetTreeStructure(const StatefulFeatureFunction* treeStructure) {
    m_treeStructure = treeStructure;
  }

  bool GetDefaultNonTermOnlyForEmptyRange() const {
    return m_defaultNonTermOnlyForEmptyRange;
  }

  S2TParsingAlgorithm GetS2TParsingAlgorithm() const {
    return m_s2tParsingAlgorithm;
  }

  bool RequireSortingAfterSourceContext() const {
    return m_requireSortingAfterSourceContext;
  }

//the setting function must be protected wih locks; the reading function does not required protection
  ttasksptr GetTask() const{
//    VERBOSE(1, "ttasksptr GetTask() const START" << std::endl);
#ifdef BOOST_HAS_PTHREADS
    pthread_t tid = pthread_self();
#else
    pthread_t tid = 0;
#endif
//    VERBOSE(1, "ttasksptr GetTask() const tid:|" << tid << "| m_ttasks.at(tid):|" << m_ttasks.at(tid) << "|" << std::endl);
//    VERBOSE(1, "ttasksptr GetTask() const just before return" << std::endl);
    return m_ttasks.at(tid);
  }

  void SetTask(ttasksptr ttask){
//    VERBOSE(1, "void SetTask(ttasksptr ttask) const START" << std::endl);
#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_ttasks_lock);
#endif

#ifdef BOOST_HAS_PTHREADS
    pthread_t tid = pthread_self();
#else
    pthread_t tid = 0;
#endif
    m_ttasks[tid] = ttask;
//    VERBOSE(1, "void SetTask(ttasksptr ttask) tid:|" << tid << "| ttask:|" << ttask << "| m_ttasks.at(tid):|" << m_ttasks.at(tid) << "|" << std::endl);
//    //    VERBOSE(1, "void SetTask(ttasksptr ttask) const END" << std::endl);
  }


};

}
#endif