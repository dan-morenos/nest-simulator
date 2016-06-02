/*
 *  connection_manager.h
 *
 *  This file is part of NEST.
 *
 *  Copyright (C) 2004 The NEST Initiative
 *
 *  NEST is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NEST is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NEST.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

// C++ includes:
#include <string>
#include <vector>

// Includes from libnestutil:
#include "manager_interface.h"

// Includes from nestkernel:
#include "conn_builder.h"
#include "gid_collection.h"
#include "nest_time.h"
#include "nest_timeconverter.h"
#include "nest_types.h"
#include "source_table.h"
#include "target_table.h"
#include "target_table_devices.h"

// Includes from sli:
#include "arraydatum.h"
#include "dict.h"
#include "dictdatum.h"

namespace nest
{
class ConnectorBase;
class HetConnector;
class GenericConnBuilderFactory;
class spikecounter;
class Node;
class Subnet;
class Event;
class SecondaryEvent;
class DelayChecker;
class GrowthCurve;
struct SpikeData;

// TODO@5g: remove tV etc.

// each thread checks delays themselves
typedef std::vector< DelayChecker > tVDelayChecker;

typedef std::vector< size_t > tVCounter; // each synapse type has a counter
// and each threads counts for all its synapses
typedef std::vector< tVCounter > tVVCounter;

class ConnectionManager : public ManagerInterface
{
  friend class SimulationManager; // update_delay_extrema_
public:
  ConnectionManager();
  virtual ~ConnectionManager();

  virtual void initialize();
  virtual void finalize();

  virtual void set_status( const DictionaryDatum& );
  virtual void get_status( DictionaryDatum& );

  DictionaryDatum& get_connruledict();

  /**
   * Add a connectivity rule, i.e. the respective ConnBuilderFactory.
   */
  template < typename ConnBuilder >
  void register_conn_builder( const std::string& name );

  ConnBuilder* get_conn_builder( const std::string& name,
    const GIDCollection& sources,
    const GIDCollection& targets,
    const DictionaryDatum& conn_spec,
    const DictionaryDatum& syn_spec );

  /**
   * Create connections.
   */
  void connect( const GIDCollection&,
    const GIDCollection&,
    const DictionaryDatum&,
    const DictionaryDatum& );

  /**
   * Connect two nodes. The source node is defined by its global ID.
   * The target node is defined by the node. The connection is
   * established on the thread/process that owns the target node.
   *
   * The parameters delay and weight have the default value NAN.
   * NAN is a special value in cmath, which describes double values that
   * are not a number. If delay or weight is omitted in a connect call,
   * NAN indicates this and weight/delay are set only, if they are valid.
   *
   * \param s GID of the sending Node.
   * \param target Pointer to target Node.
   * \param target_thread Thread that hosts the target node.
   * \param syn The synapse model to use.
   * \param d Delay of the connection (in ms).
   * \param w Weight of the connection.
   */
  void connect( index s,
    Node* target,
    thread target_thread,
    index syn,
    double_t d = NAN,
    double_t w = NAN );

  /**
   * Connect two nodes. The source node is defined by its global ID.
   * The target node is defined by the node. The connection is
   * established on the thread/process that owns the target node.
   *
   * The parameters delay and weight have the default value NAN.
   * NAN is a special value in cmath, which describes double values that
   * are not a number. If delay or weight is omitted in an connect call,
   * NAN indicates this and weight/delay are set only, if they are valid.
   *
   * \param s GID of the sending Node.
   * \param target Pointer to target Node.
   * \param target_thread Thread that hosts the target node.
   * \param syn The synapse model to use.
   * \param params parameter dict to configure the synapse
   * \param d Delay of the connection (in ms).
   * \param w Weight of the connection.
   */
  void connect( index s,
    Node* target,
    thread target_thread,
    index syn,
    DictionaryDatum& params,
    double_t d = NAN,
    double_t w = NAN );

  /**
   * Connect two nodes. The source node is defined by its global ID.
   * The target node is defined by the node. The connection is
   * established on the thread/process that owns the target node.
   *
   * \param s GID of the sending Node.
   * \param target pointer to target Node.
   * \param target_thread thread that hosts the target node
   * \param params parameter dict to configure the synapse
   * \param syn The synapse model to use.
   */
  bool connect( index s, index r, DictionaryDatum& params, index syn );

  void
  disconnect( Node& target, index sgid, thread target_thread, index syn_id );

  void subnet_connect( Subnet&, Subnet&, int, index syn );

  /**
   * Connect from an array of dictionaries.
   */
  bool connect( ArrayDatum& connectome );

  void divergent_connect( index s,
    const TokenArray& r,
    const TokenArray& weights,
    const TokenArray& delays,
    index syn );
  /**
   * Connect one source node with many targets.
   * The dictionary d contains arrays for all the connections of type syn.
   */

  void divergent_connect( index s, DictionaryDatum d, index syn );

  void random_divergent_connect( index s,
    const TokenArray& r,
    index n,
    const TokenArray& w,
    const TokenArray& d,
    bool,
    bool,
    index syn );

  void convergent_connect( const TokenArray& s,
    index r,
    const TokenArray& weights,
    const TokenArray& delays,
    index syn );

  /**
   * Specialized version of convegent_connect
   * called by random_convergent_connect threaded
   */
  void convergent_connect( const std::vector< index >& s_id,
    index r,
    const TokenArray& weight,
    const TokenArray& delays,
    index syn );

  void random_convergent_connect( const TokenArray& s,
    index t,
    index n,
    const TokenArray& w,
    const TokenArray& d,
    bool,
    bool,
    index syn );

  /**
   * Use openmp threaded parallelization to speed up connection.
   * Parallelize over target list.
   */
  void random_convergent_connect( TokenArray& s,
    TokenArray& t,
    TokenArray& n,
    TokenArray& w,
    TokenArray& d,
    bool,
    bool,
    index syn );

  // aka conndatum GetStatus
  DictionaryDatum
  get_synapse_status( const index source_gid,
    const index target_gid,
    const thread tid,
    const synindex syn_id,
    const port p ) const;

  // aka conndatum SetStatus
  void set_synapse_status( const index source_gid,
    const index target_gid,
    const thread tid,
    const synindex syn_id,
    const port p,
    const DictionaryDatum& dict );

  /**
   * Return connections between pairs of neurons.
   * The params dictionary can have the following entries:
   * 'source' a token array with GIDs of source neurons.
   * 'target' a token array with GIDs of target neuron.
   * If either of these does not exist, all neuron are used for the respective
   * entry.
   * 'synapse_model' name of the synapse model, or all synapse models are
   * searched.
   * 'synapse_label' label (long_t) of the synapse, or all synapses are
   * searched.
   * The function then iterates all entries in source and collects the
   * connection IDs to all neurons in target.
   */
  ArrayDatum get_connections( DictionaryDatum dict ) const;

  void get_connections( ArrayDatum& connectome,
    TokenArray const* source,
    TokenArray const* target,
    synindex syn_id,
    long_t synapse_label ) const;

  /**
   * Returns the number of connections in the network.
   */
  size_t get_num_connections() const;

  /**
   * Returns the number of connections of this synapse type.
   */
  size_t get_num_connections( synindex syn_id ) const;

  void get_sources( std::vector< index > targets,
    std::vector< std::vector< index > >& sources,
    index synapse_model );
  void get_targets( std::vector< index > sources,
    std::vector< std::vector< index > >& targets,
    index synapse_model );

  const std::vector< Target >& get_targets( const thread tid, const index lid ) const;

  index get_target_gid( const thread tid,
    const synindex syn_index,
    const index lcid ) const;

  /**
   * Triggered by volume transmitter in update.
   * Triggeres updates for all connectors of dopamine synapses that
   * are registered with the volume transmitter with gid vt_gid.
   */
  void trigger_update_weight( const long_t vt_gid,
    const std::vector< spikecounter >& dopa_spikes,
    const double_t t_trig );

  /**
   * Return minimal connection delay, which is precomputed by
   * update_delay_extrema_().
   */
  delay get_min_delay() const;

  /**
   * Return maximal connection delay, which is precomputed by
   * update_delay_extrema_().
   */
  delay get_max_delay() const;

  bool get_user_set_delay_extrema() const;

  void send( thread t, index sgid, Event& e );

  void send_secondary( thread t, SecondaryEvent& e );

  void send_5g( const thread tid,
    const synindex syn_index,
    const index lcid,
    Event& e );

  /**
   * Send event e to all device targets of source s_gid
   */
  void send_to_devices( const thread tid, const index source_gid, Event& e );

  /**
   * Send event e to all targets of source device ldid (local device id)
   */
  void send_from_device( const thread tid, const index ldid, Event& e );

  /**
   * Send event e to all targets of node source on thread t
   */
  void send_local( thread t, Node& source, Event& e );

  /**
   * Resize the structures for the Connector objects if necessary.
   * This function should be called after number of threads, min_delay,
   * max_delay, and time representation have been changed in the scheduler.
   * The TimeConverter is used to convert times from the old to the new
   * representation. It is also forwarding the calibration request to all
   * ConnectorModel objects.
   */
  void calibrate( const TimeConverter& );

  /**
   * Returns the delay checker for the current thread.
   */
  DelayChecker& get_delay_checker();

  bool is_source_table_cleared() const;

  void prepare_target_table( const thread tid );

  void resize_target_table_devices();

  bool get_next_target_data( const thread tid,
    const thread rank_start,
    const thread rank_end,
    thread& target_rank,
    TargetData& next_target_data );

  void reject_last_target_data( const thread tid );

  void save_source_table_entry_point( const thread tid );

  void reset_source_table_entry_point( const thread tid );

  void restore_source_table_entry_point( const thread tid );

  void add_target( const thread tid, const TargetData& target_data);

  void sort_connections();

  bool have_connections_changed() const;
  void set_have_connections_changed( const bool changed );

  void restructure_connection_tables();

private:
  /**
   * Update delay extrema to current values.
   *
   * Static since it only operates in static variables. This allows it to be
   * called from const-method get_status() as well.
   */
  void update_delay_extrema_();

  /**
   * This method queries and finds the minimum delay
   * of all local connections
   */
  const Time get_min_delay_time_() const;

  /**
   * This method queries and finds the minimum delay
   * of all local connections
   */
  const Time get_max_delay_time_() const;

  /**
   * Deletes all connections.
   */
  void delete_connections_5g_();

  /**
   * connect_ is used to establish a connection between a sender and
   * receiving node which both have proxies.
   *
   * The parameters delay and weight have the default value NAN.
   * NAN is a special value in cmath, which describes double values that
   * are not a number. If delay or weight is omitted in an connect call,
   * NAN indicates this and weight/delay are set only, if they are valid.
   *
   * \param s A reference to the sending Node.
   * \param r A reference to the receiving Node.
   * \param s_gid The global id of the sending Node.
   * \param tid The thread of the target node.
   * \param syn The synapse model to use.
   * \param d The delay of the connection (optional).
   * \param w The weight of the connection (optional).
   * \param p The parameters for the connection.
   */
  void connect_( Node& s,
    Node& r,
    index s_gid,
    thread tid,
    index syn,
    double_t d = NAN,
    double_t w = NAN );
  void connect_( Node& s,
    Node& r,
    index s_gid,
    thread tid,
    index syn,
    DictionaryDatum& p,
    double_t d = NAN,
    double_t w = NAN );

  /**
   * connect_to_device_ is used to establish a connection between a sender and
   * receiving node if the sender has proxies, and the receiver does not.
   *
   * The parameters delay and weight have the default value NAN.
   * NAN is a special value in cmath, which describes double values that
   * are not a number. If delay or weight is omitted in an connect call,
   * NAN indicates this and weight/delay are set only, if they are valid.
   *
   * \param s A reference to the sending Node.
   * \param r A reference to the receiving Node.
   * \param s_gid The global id of the sending Node.
   * \param tid The thread of the target node.
   * \param syn The synapse model to use.
   * \param d The delay of the connection (optional).
   * \param w The weight of the connection (optional).
   * \param p The parameters for the connection.
   */
  void connect_to_device_( Node& s,
    Node& r,
    index s_gid,
    thread tid,
    index syn,
    double_t d = NAN,
    double_t w = NAN );
  void connect_to_device_( Node& s,
    Node& r,
    index s_gid,
    thread tid,
    index syn,
    DictionaryDatum& p,
    double_t d = NAN,
    double_t w = NAN );

  /**
   * connect_from_device_ is used to establish a connection between a sender and
   * receiving node if the sender has proxies, and the receiver does not.
   *
   * The parameters delay and weight have the default value NAN.
   * NAN is a special value in cmath, which describes double values that
   * are not a number. If delay or weight is omitted in an connect call,
   * NAN indicates this and weight/delay are set only, if they are valid.
   *
   * \param s A reference to the sending Node.
   * \param r A reference to the receiving Node.
   * \param s_gid The global id of the sending Node.
   * \param tid The thread of the target node.
   * \param syn The synapse model to use.
   * \param d The delay of the connection (optional).
   * \param w The weight of the connection (optional).
   * \param p The parameters for the connection.
   */
  void connect_from_device_( Node& s,
    Node& r,
    index s_gid,
    thread tid,
    index syn,
    double_t d = NAN,
    double_t w = NAN );
  void connect_from_device_( Node& s,
    Node& r,
    index s_gid,
    thread tid,
    index syn,
    DictionaryDatum& p,
    double_t d = NAN,
    double_t w = NAN );

  /** A structure to hold the Connector objects which in turn hold the
   * connection information. Corresponds to a three dimensional
   * structure: threads|synapses|connections */
  std::vector< HetConnector* > connections_5g_;

  /**
   * A structure to hold the global ids of presynaptic neurons during
   * postsynaptic connection creation, before the connection
   * information has been transferred to the presynaptic side.
   * Internally arragend in a 3d structure: threads|synapses|gids
   */
  SourceTable source_table_;

  /** A structure to hold the information about targets for each
   * neuron on the presynaptic side. Internally arranged in a 3d
   * structure: threads|localnodes|targets
   */
  TargetTable target_table_;

  TargetTableDevices target_table_devices_;

  tVDelayChecker delay_checkers_;

  /** A structure to count the number of synapses of a specific
   * type. Arranged in a 2d structure: threads|synapsetypes.
   */
  tVVCounter vv_num_connections_;

  /**
   * BeginDocumentation
   * Name: connruledict - dictionary containing all connectivity rules
   * Description:
   * This dictionary provides the connection rules that can be used
   * in Connect.
   * 'connruledict info' shows the contents of the dictionary.
   * SeeAlso: Connect
   */
  DictionaryDatum connruledict_; //!< Dictionary for connection rules.

  //! ConnBuilder factories, indexed by connruledict_ elements.
  std::vector< GenericConnBuilderFactory* > connbuilder_factories_;

  delay min_delay_; //!< Value of the smallest delay in the network.

  delay max_delay_; //!< Value of the largest delay in the network in steps.

  bool keep_source_table_;  //!< Whether to keep source table after connection setup is complete

  bool have_connections_changed_; //!< true if new connections have been created
                                  //!< since startup or last call to simulate
};

inline DictionaryDatum&
ConnectionManager::get_connruledict()
{
  return connruledict_;
}

inline delay
ConnectionManager::get_min_delay() const
{
  return min_delay_;
}

inline delay
ConnectionManager::get_max_delay() const
{
  return max_delay_;
}

inline bool
ConnectionManager::is_source_table_cleared() const
{
  return source_table_.is_cleared();
}

inline void
ConnectionManager::resize_target_table_devices()
{
  target_table_devices_.resize();
}

inline void
ConnectionManager::reject_last_target_data( const thread tid )
{
  source_table_.reject_last_target_data( tid );
}

inline void
ConnectionManager::save_source_table_entry_point( const thread tid )
{
  source_table_.save_entry_point( tid );
}

inline void
ConnectionManager::reset_source_table_entry_point( const thread tid )
{
  source_table_.reset_entry_point( tid );
}

inline void
ConnectionManager::restore_source_table_entry_point( const thread tid )
{
  source_table_.restore_entry_point( tid );
}

inline void
ConnectionManager::prepare_target_table( const thread tid )
{
  target_table_.prepare( tid );
}

inline const std::vector< Target >&
ConnectionManager::get_targets( const thread tid, const index lid ) const
{
  return target_table_.get_targets( tid, lid );
}

inline bool
ConnectionManager::have_connections_changed() const
{
  return have_connections_changed_;
}

inline void
ConnectionManager::set_have_connections_changed( const bool changed )
{
  have_connections_changed_ = changed;
}

} // namespace nest

#endif /* CONNECTION_MANAGER_H */
