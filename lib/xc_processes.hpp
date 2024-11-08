// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file xc_processes.hpp
 * @brief Case study on XC processes.
 */

#ifndef FCPP_XC_PROCESSES_H_
#define FCPP_XC_PROCESSES_H_

#include <iostream>

#include "lib/common/option.hpp"
#include "lib/component/calculus.hpp"
#include "lib/option/distribution.hpp"

#include "lib/generals.hpp"
#include "lib/xc_setup.hpp"

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief Handles a process, spawning instances of it for every key in the `key_set` and passing general arguments `xs` (overload with field<bool> status).
template <typename node_t, typename G, typename S, typename... Ts, typename K = typename std::decay_t<S>::value_type, typename T = std::decay_t<std::result_of_t<G(K const&, Ts const&...)>>, typename R = std::decay_t<tuple_element_t<0,T>>, typename B = std::decay_t<tuple_element_t<1,T>>>
std::enable_if_t<std::is_same<B,field<bool>>::value, std::unordered_map<K, R>>
spawn(node_t& node, trace_t call_point, G&& process, S&& key_set, Ts const&... xs) {
    return spawn(node, call_point, [&](K const& k, auto const&... params){
        return nbr(node, call_point, field<bool>(false), [&](field<bool> n){
            bool b = false;
            R ret;
            field<bool> fb = false;

            bool found=false;
            for (auto it = key_set.cbegin(); it < key_set.cend(); it++) {
                if (*it == k) {
                    found = true;
                    break;
                }
            }

            if (coordination::any_hood(node, call_point, n) or found) {
                tie(ret, fb) = process(k, params...);
                b = coordination::any_hood(node, call_point, fb) or other(fb);
            }
            return make_tuple(make_tuple(ret, b), fb);
        });
    }, std::forward<S>(key_set), xs...);
}

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

template <typename T>
using nvalue = field<T>;

//! @brief Length of a round
constexpr size_t period = 1;

//! @brief Communication radius.
constexpr size_t comm = 100;


//! @brief Possibly generates a message, given the number of devices and the experiment tag.
FUN common::option<message> get_message(ARGS, size_t devices) {
    common::option<message> m;
    #ifndef MULTI_TEST
    bool genmsg = node.uid == devices-1 && node.current_time() > 10 && node.storage(tags::sent_count{}) == 0;
    #else
    bool genmsg = node.uid >= devices-10 && node.current_time() > 1 && node.current_time() < 26 && node.next_real() < 0.05;
    #endif
    // random message with 1% probability during time [10..50]
    if (genmsg) {
        m.emplace(node.uid, (device_t)node.next_int(devices-1), node.current_time(), node.next_real());
        node.storage(tags::sent_count{}) += 1;
    }
    return m;
}

//! @brief Result type of spawn calls dispatching messages.
// TODO ****check --> should be size_t
using message_log_type = std::unordered_map<message, double, fcpp::common::hash<message>>;
//using message_log_type = std::unordered_map<message, bool>;

//! @brief Computes stats on message delivery and active processes.
GEN(T) void proc_stats(ARGS, message_log_type const& nm, bool render, T) {
    // import tags for convenience
    using namespace tags;
    // stats on number of active processes
    int proc_num = node.storage(proc_data{}).size() - 1;
#ifdef ALLPLOTS
    node.storage(max_proc<T>{}) = max(node.storage(max_proc<T>{}), proc_num);
#endif
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
        for (auto const& x : nm) if (x.first.to == node.uid) {
            if (m.count(x.first)) {
#ifdef ALLPLOTS
                node.storage(repeat_count<T>{}) += 1;
#endif
            } else {
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

//! @brief Wrapper calling a spawn function with a given process and key set, while tracking the processes executed.
GEN(T,G,S) void spawn_profiler(ARGS, T, G&& process, S&& key_set, real_t v, bool render) {
    // clear up stats data
    node.storage(tags::proc_data{}).clear();
    node.storage(tags::proc_data{}).push_back(color::hsva(0, 0, 0.3, 1));
    // dispatches messages
    message_log_type r = spawn(node, call_point, [&](message const& m){
        auto r = process(m, v);
        // TODO **** adapt to field<bool>
        //real_t key = get<0>(r) == status::external ? 0.5 : 1;
        real_t key = get<0>(r) ? 0.5 : 1;
        //real_t key = 1;
        node.storage(tags::proc_data{}).push_back(color::hsva(m.data * 360, key, key));
        return r;
    }, std::forward<S>(key_set));

    // compute stats
    proc_stats(CALL, r, render, T{});
}
//! @brief Export list for spawn_profiler.
FUN_EXPORT spawn_profiler_t = export_list<spawn_t<message, bool>, spawn_t<message, status>, proc_stats_t, nvalue<bool>>;

//! @brief Makes test for spherical processes.
GEN(T) void spherical_test(ARGS, common::option<message> const& m, T, bool render = false) { CODE
    spawn_profiler(CALL, tags::spherical<T>{}, [&](message const& m, real_t v){
        int rnd = counter(CALL);
        nvalue<bool> fdwav = false;

        if (m.to != node.uid and rnd <= 2) {
            fdwav = mod_self(CALL, fdwav, true);
            fdwav = mod_other(CALL, fdwav, rnd == 1);
        }

        return make_tuple(node.current_time(), fdwav);

    }, m, node.storage(tags::infospeed{}), render);
}
FUN_EXPORT spherical_test_t = export_list<spawn_profiler_t, double, monotonic_distance_t, bool, int>;

using set_t = std::unordered_set<device_t>;

//! @brief Makes test for tree processes.
GEN(T,S) void tree_test(ARGS, common::option<message> const& m, nvalue<device_t> fdneigh, nvalue<device_t> fdparent, nvalue<S> const& fdbelow, size_t set_size, T, int render = -1) { CODE
    spawn_profiler(CALL, tags::tree<T>{}, [&](message const& m, real_t v){
        int rnd = counter(CALL);
        nvalue<bool> fdwav = false;

        if (m.to != node.uid and rnd <= 2) {
            nvalue<bool> source_path  = map_hood([&] (device_t d) {return (d == self(CALL, fdparent));}, fdneigh);
            nvalue<bool> dest_path = map_hood([&] (device_t d) {return (d == node.uid);}, fdparent) and map_hood([&] (S b) {return (b.count(m.to) > 0);}, fdbelow);

            fdwav = source_path or dest_path;
            fdwav = mod_self(CALL, fdwav, rnd == 1);
        }

        return make_tuple(node.current_time(), fdwav);

    }, m, 0.3, render);
}
FUN_EXPORT tree_test_t = export_list<spawn_profiler_t, double, monotonic_distance_t, bool, int>;


//! @brief Standard deviation for distance estimations.
constexpr size_t dist_dev = 30;
//! @brief Generating distribution for distance estimations.
std::weibull_distribution<real_t> dist_distr = distribution::make<std::weibull_distribution>(real_t(1), real_t(dist_dev*0.01));

//! @brief Adjusted nbr_dist value accounting for errors.
FUN field<real_t> adjusted_nbr_dist(ARGS) {
    return node.nbr_dist() * rand_hood(CALL, dist_distr) + node.storage(tags::speed{}) * comm / period * node.nbr_lag();
}
//! @brief Wave-like termination logic.
template <typename node_t, template<class> class T>
void termination_logic(ARGS, status& s, real_t v, message const& m, T<tags::wispp>) {
    bool terminating = s == status::terminated_output;
    bool terminated = nbr(CALL, terminating, [&](field<bool> nt){
        return any_hood(CALL, nt) or terminating;
    });
    bool source = m.from == node.uid and old(CALL, true, false);
    double ds = monotonic_distance(CALL, source, adjusted_nbr_dist(CALL));
    double dt = monotonic_distance(CALL, source, node.nbr_lag());
    bool slow = ds < v * comm / period * (dt - period);
    if (terminated or slow) {
        if (s == status::terminated_output) s = status::border_output;
        if (s == status::internal) s = status::border;
        if (s == status::internal_output) s = status::border_output;
    }
}

//! @brief Makes test for FC tree processes.
GEN(T,S) void fc_tree_test(ARGS, common::option<message> const& m, device_t parent, S const& below, size_t set_size, T, int render = -1) { CODE
    spawn_profiler(CALL, tags::tree<T>{}, [&](message const& m, real_t v){
        bool source_path = any_hood(CALL, nbr(CALL, parent) == node.uid) or node.uid == m.from;
        bool dest_path = below.count(m.to) > 0;
        status s = node.uid == m.to ? status::terminated_output :
                   source_path or dest_path ? status::internal : status::external_deprecated;
        termination_logic(CALL, s, v, m, tags::tree<tags::wispp>{});
        return make_tuple(node.current_time(), s);
    }, m, 0.3, render);
}
//! @brief Exports for the main function.
FUN_EXPORT fc_tree_test_t = export_list<spawn_profiler_t>;

using set_t = std::unordered_set<device_t>;

//! @brief Main case study function.
MAIN() {
    // import tags for convenience
    using namespace tags;
    // random walk
    size_t l = node.storage(side{});
    rectangle_walk(CALL, make_vec(0,0,20), make_vec(l,l,20), node.storage(speed{}) * comm / period, 1);

    #ifndef NOTREE
    bool is_src = node.uid == 0;
    #else
    bool is_src = false;
    #endif

    bool highlight = is_src or node.uid == node.storage(devices{}) - 1;
    node.storage(node_shape{}) = is_src ? shape::icosahedron : highlight ? shape::cube : shape::sphere;
    node.storage(node_size{}) = highlight ? 20 : 10;
    // random message with 1% probability during time [10..50]
    common::option<message> m = get_message(CALL, node.storage(devices{}));

    #ifndef NOSPHERE
    spherical_test(CALL, m, xc{}, true);
    #endif
    #ifndef NOTREE
    // spanning tree definition
    device_t parent = flex_parent(CALL, is_src, comm);
    // routing sets along the tree
    set_t below = parent_collection(CALL, parent, set_t{node.uid}, [](set_t x, set_t const& y){
        x.insert(y.begin(), y.end());
        return x;
    });
    nvalue<set_t> fdbelow = nbr(CALL, below); 
    nvalue<device_t> fdparent = nbr(CALL, parent); 
    nvalue<device_t> fdneigh = nbr_uid(CALL);

    common::osstream os;
    os << below;

    tree_test(CALL, m, fdneigh, fdparent, fdbelow, os.size(), xc{});
    fc_tree_test(CALL, m, parent, below, os.size(), fc{});

    #endif

}
//! @brief Exports for the main function.
struct main_t : public export_list<rectangle_walk_t<3>, spherical_test_t, tree_test_t, flex_parent_t, parent_collection_t<set_t>, real_t> {};


} // coordination

} // fcpp

#endif // FCPP_PROCESS_MANAGEMENT_H_
