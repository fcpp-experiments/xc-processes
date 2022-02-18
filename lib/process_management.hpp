// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file process_management.hpp
 * @brief Case study on process lifetime management.
 */

#ifndef FCPP_PROCESS_MANAGEMENT_H_
#define FCPP_PROCESS_MANAGEMENT_H_

#include "lib/common/option.hpp"
#include "lib/component/calculus.hpp"
#include "lib/generals.hpp"


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {


//! @brief Length of a round
constexpr size_t period = 1;

//! @brief Communication radius.
constexpr size_t comm = 100;

//! @brief Maximum discrepancy between space and time
constexpr real_t timespace_threshold = 2.0 * comm / period;


//! @brief Possibly generates a message, given the number of devices and the experiment tag.
FUN common::option<message> get_message(ARGS, size_t devices) {
    common::option<message> m;
    // random message with 1% probability during time [10..50]
    if (node.uid == 0 && node.current_time() > 1 && node.storage(tags::sent_count{}) == 0) {
//    if (node.current_time() > 1 and node.current_time() < 25 and node.next_real() < 0.01) {
        m.emplace(node.uid, (device_t)node.next_int(devices-1), node.current_time(), node.next_real());
        node.storage(tags::sent_count{}) += 1;
    }
    return m;
}


//! @brief Result type of spawn calls dispatching messages.
using message_log_type = std::unordered_map<message, times_t>;


//! @brief Computes stats on message delivery and active processes.
GEN(T) void proc_stats(ARGS, message_log_type const& nm, bool render, T) {
    // import tags for convenience
    using namespace tags;
    // stats on number of active processes
    int proc_num = node.storage(proc_data{}).size() - 1;
    node.storage(max_proc<T>{}) = max(node.storage(max_proc<T>{}), proc_num);
    node.storage(tot_proc<T>{}) += proc_num;
    // additional node rendering
    if (render) {
        if (proc_num > 0) node.storage(node_size{}) *= 1.5;
        node.storage(node_color{})  = node.storage(proc_data{})[min(proc_num, 1)];
        node.storage(left_color{})  = node.storage(proc_data{})[min(proc_num, 2)];
        node.storage(right_color{}) = node.storage(proc_data{})[min(proc_num, 3)];
    }
    // stats on delivery success
    old(node, call_point, message_log_type{}, [&](message_log_type m){
        for (auto const& x : nm) {
            if (m.count(x.first)) node.storage(repeat_count<T>{}) += 1;
            else {
                node.storage(first_delivery_tot<T>{}) += x.second - x.first.time;
                node.storage(delivery_count<T>{}) += 1;
                m[x.first] = x.second;
            }
        }
        return m;
    });
}
//! @brief Export list for proc_stats.
FUN_EXPORT proc_stats_t = export_list<message_log_type>;


//! @brief Legacy termination logic (COORD19).
template <typename node_t, template<class> class T>
void termination_logic(ARGS, status& s, real_t, message const&, T<tags::legacy>) {
     bool terminating = s == status::terminated_output;
     bool terminated = old(CALL, terminating, [&](bool ot){
        return any_hood(CALL, nbr(CALL, ot), ot) or terminating;
     });
    bool exiting = all_hood(CALL, nbr(CALL, terminated), terminated);

    if (exiting) s = status::external;
    else if (terminating) s = status::internal_output;
}
//! @brief Legacy termination logic updated to use share (LMCS2020) instead of rep+nbr.
template <typename node_t, template<class> class T>
void termination_logic(ARGS, status& s, real_t, message const&, T<tags::share>) {
    /*
    // todo REMOVE
    if (s == status::terminated_output)
	std::cout << node.uid << " TERMINATED" << std::endl;
    */
    bool terminating = s == status::terminated_output;
    bool terminated = nbr(CALL, terminating, [&](field<bool> nt){
        return any_hood(CALL, nt) or terminating;
    });
    bool exiting = all_hood(CALL, nbr(CALL, terminated), terminated);
    /*
    // todo REMOVE
    if (exiting)
	std::cout << node.uid << " EXITING" << std::endl;
    */
    if (exiting) s = status::external;
    else if (terminating) s = status::internal_output;
}
//! @brief Novel termination logic.
template <typename node_t, template<class> class T>
void termination_logic(ARGS, status& s, real_t ds, message const& m, T<tags::novel>) {
    times_t dt = nbr(CALL, INF, [&](field<times_t> ndt){
        times_t t = min_hood(CALL, ndt + node.nbr_lag());
        return node.uid == m.from ? 0 : t;
    });
    bool terminating = s == status::terminated_output;
    bool terminated = nbr(CALL, terminating, [&](field<bool> nt){
        return any_hood(CALL, nt) or terminating;
    });
    if (terminated or ds < timespace_threshold * (dt - period)) {
        if (s == status::terminated_output) s = status::border_output;
        if (s == status::internal) s = status::border;
    }
}
//! @brief Wave-like termination logic.
template <typename node_t, template<class> class T>
void termination_logic(ARGS, status& s, real_t ds, message const& m, T<tags::wave>) {
    times_t dt = nbr(CALL, INF, [&](field<times_t> ndt){
        times_t t = min_hood(CALL, ndt + node.nbr_lag());
        return node.uid == m.from and counter(CALL) == 1 ? 0 : t;
    });
    bool terminating = s == status::terminated_output;
    bool terminated = nbr(CALL, terminating, [&](field<bool> nt){
        return any_hood(CALL, nt) or terminating;
    });
    if (terminated or ds < timespace_threshold * (dt - period)) {
        if (s == status::terminated_output) s = status::border_output;
        if (s == status::internal) s = status::border;
    }
    /*
    real_t md = max_hood(CALL, nbr(CALL, ds), ds);
    if (md <= old(CALL, -INF, md)) {
        if (s == status::border_output) s = status::external_output;
        else s = status::external;
    }
    */
}
//! @brief Export list for termination_logic.
FUN_EXPORT termination_logic_t = export_list<bool, counter_t<>, real_t>;


//! @brief Wrapper calling a spawn function with a given process and key set, while tracking the processes executed.
GEN(T,G,S) void spawn_profiler(ARGS, T, G&& process, S&& key_set, bool render) {
    // clear up stats data
    node.storage(tags::proc_data{}).clear();
    node.storage(tags::proc_data{}).push_back(color(BLACK));
    // dispatches messages
    message_log_type r = spawn(node, call_point, [&](message const& m){
        node.storage(tags::proc_data{}).push_back(color::hsva(m.data * 360, 1, 1));
        double ds = monotonic_distance(CALL, m.from == node.uid);
        auto r = process(m, ds);
        termination_logic(CALL, get<1>(r), ds, m, T{});
        return r;
    }, std::forward<S>(key_set));
    // compute stats
    proc_stats(CALL, r, render, T{});
}
//! @brief Export list for spawn_profiler.
FUN_EXPORT spawn_profiler_t = export_list<spawn_t<message, status>, termination_logic_t, monotonic_distance_t, proc_stats_t>;


//! @brief Makes test for spherical processes.
GEN(T) void spherical_test(ARGS, common::option<message> const& m, real_t max_distance, T, bool render = false) { CODE
    spawn_profiler(CALL, tags::spherical<T>{}, [&](message const& m, real_t ds){
        status s = node.uid == m.to ? status::terminated_output :
                   ds < max_distance ? status::internal : status::external;
	return make_tuple(node.current_time(), s);
    }, m, render);
}
FUN_EXPORT spherical_test_t = export_list<spawn_profiler_t>;


//! @brief The type for a set of devices.
using set_t = std::unordered_set<device_t>;

//! @brief Makes test for tree processes.
GEN(T) void tree_test(ARGS, common::option<message> const& m, device_t parent, set_t const& below, T, bool render = false) { CODE
    spawn_profiler(CALL, tags::tree<T>{}, [&](message const& m, real_t){
        bool inpath = below.count(m.from) + below.count(m.to) > 0;
        status s = node.uid == m.to ? status::terminated_output :
                   inpath ? status::internal : status::external;
        return make_tuple(node.current_time(), s);
    }, m, render);
}
//! @brief Exports for the main function.
FUN_EXPORT tree_test_t = export_list<spawn_profiler_t>;


//! @brief Main case study function.
MAIN() {
    // import tags for convenience
    using namespace tags;
    // random walk
    size_t l = node.storage(side{});
    rectangle_walk(CALL, make_vec(0,0,20), make_vec(l,l,20), node.storage(speed{}), 1);
    // basic node rendering
    bool is_src = node.uid == 0;
    node.storage(node_shape{}) = is_src ? shape::cube : shape::sphere;
    node.storage(node_size{}) = is_src ? 16 : 10;
    // random message with 1% probability during time [10..50]
    common::option<message> m = get_message(CALL, node.storage(devices{}));
#ifndef NOSPHERE
    // tests spherical processes with legacy termination
    spherical_test(CALL, m, INF, legacy{});
    spherical_test(CALL, m, INF, share{});
    spherical_test(CALL, m, INF, novel{});
    spherical_test(CALL, m, INF, wave{});
#endif
#ifndef NOTREE
    // spanning tree definition
    double ds = bis_distance(CALL, is_src, 1, 100);
    //    double ds = flex_distance(CALL, is_src, 1, 100);
    device_t parent = get<1>(min_hood(CALL, make_tuple(nbr(CALL, ds), node.nbr_uid())));
    // routing sets along the tree
    set_t below = sp_collection(CALL, ds, set_t{node.uid}, set_t{}, [](set_t x, set_t const& y){
        x.insert(y.begin(), y.end());
        return x;
    });
    // test tree processes with legacy termination
    tree_test(CALL, m, parent, below, legacy{});
    tree_test(CALL, m, parent, below, share{});
    tree_test(CALL, m, parent, below, novel{});
    tree_test(CALL, m, parent, below, wave{});
#endif
}
//! @brief Exports for the main function.
struct main_t : public export_list<rectangle_walk_t<3>, spherical_test_t, bis_distance_t, real_t, sp_collection_t<double, set_t>, tree_test_t> {};


} // coordination

} // fcpp

#endif // FCPP_PROCESS_MANAGEMENT_H_
