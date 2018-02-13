#include "branch_length_estimator.hpp"

//////////////////////////////////////////


EstimateBranchLengths::EstimateBranchLengths(const Data& data){
  N       = data.N;
  N_total = 2*N - 1;
  L       = data.L;
  Ne      = data.Ne;

  coordinates.resize(N_total);
  sorted_indices.resize(N_total); //node indices in order of coalescent events
  order.resize(N_total); //order of coalescent events
};


//MCMC
void
EstimateBranchLengths::InitializeBranchLengths(Tree& tree){

  int node_i, num_lineages;
  //initialize using coalescent prior
  coordinates.resize(N_total);
  for(int i = 0; i < N; i++){
    coordinates[i] = 0.0;
  }
  for(int i = N; i < N_total; i++){
    num_lineages = 2*N-i;
    node_i = sorted_indices[i];
    coordinates[node_i] = coordinates[sorted_indices[i-1]] + 2.0/( num_lineages * (num_lineages - 1.0) ); // determined by the prior
    (*tree.nodes[node_i].child_left).branch_length  = coordinates[node_i] - coordinates[(*tree.nodes[node_i].child_left).label];
    (*tree.nodes[node_i].child_right).branch_length = coordinates[node_i] - coordinates[(*tree.nodes[node_i].child_right).label];
  }

}

void
EstimateBranchLengths::InitializeMCMC(const Data& data, Tree& tree){

  mut_rate.resize(N_total);
  for(int i = 0; i < N_total; i++){
    int snp_begin = tree.nodes[i].SNP_begin;
    int snp_end   = tree.nodes[i].SNP_end;

    //if(snp_end >= data.pos.size()) snp_end = data.pos.size()-1;
    mut_rate[i]            = 0.0;
    for(int snp = snp_begin; snp < snp_end; snp++){
      mut_rate[i]         += data.pos[snp];
    }

    if(snp_begin > 0){
      snp_begin--;
      mut_rate[i]         += 0.5 * data.pos[snp_begin];
    }
    if(snp_end < data.L-1){
      mut_rate[i]         += 0.5 * data.pos[snp_end];
    }

    mut_rate[i]           *= data.Ne * data.mu;
    //mut_rate[i]     = 0.0;
  }

  //initialize
  //1. sort coordinate vector to obtain sorted_indices
  //2. sort sorted_indices to obtain order

  order.resize(N_total);
  sorted_indices.resize(N_total);

  for(int i = 0; i < N_total; i++){
    order[i] = i;
    sorted_indices[i] = i;
  }

  //debug
  //for(int i = 0; i < N_total-1; i++){
  //  assert(order[i] < order[(*tree.nodes[i].parent).label]);
  //}

}

void
EstimateBranchLengths::UpdateAvg(Tree& tree){


  if(update_node1 != -1){

    if(update_node2 != -1){

      it_avg         = std::next(avg.begin(), update_node1);
      it_coords      = std::next(coordinates.begin(), update_node1);
      it_last_update = std::next(last_update.begin(), update_node1);
      it_last_coords = std::next(last_coordinates.begin(), update_node1);
      *it_avg         += ((count - *it_last_update) * (*it_last_coords - *it_avg) + *it_coords - *it_last_coords)/count;
      *it_last_update  = count;
      *it_last_coords  = *it_coords;

      it_avg         = std::next(avg.begin(), update_node2);
      it_coords      = std::next(coordinates.begin(), update_node2);
      it_last_update = std::next(last_update.begin(), update_node2);
      it_last_coords = std::next(last_coordinates.begin(), update_node2);
      *it_avg         += ((count - *it_last_update) * (*it_last_coords - *it_avg) + *it_coords - *it_last_coords)/count;
      *it_last_update  = count;
      *it_last_coords  = *it_coords;

      update_node1 = -1;
      update_node2 = -1;

    }else{

      avg[update_node1]             += ((count - last_update[update_node1]) * (last_coordinates[update_node1] - avg[update_node1]) + coordinates[update_node1] - last_coordinates[update_node1])/count;
      last_update[update_node1]      = count;
      last_coordinates[update_node1] = coordinates[update_node1];

      update_node1 = -1;

    }

  }


  /*  
      it_avg = std::next(avg.begin(), N);
      it_coords = std::next(coordinates.begin(), N);
  //update avg
  for(int ell = N; ell < N_total; ell++){
   *it_avg  += (*it_coords - *it_avg)/count;
   last_update[ell] = count;
   it_avg++;
   it_coords++;
   }
   */


}


//////////////////////////////////////////

void
EstimateBranchLengths::RandomSwitchOrder(Tree& tree, int k, std::uniform_real_distribution<double>& dist_unif){

  int node_k, node_swap_k;

  //check order of parent and order of children
  //choose a random number in this range (not including)
  //swap order with that node

  node_k = sorted_indices[k];

  int parent_order    = order[(*tree.nodes[node_k].parent).label];
  int child_order     = order[(*tree.nodes[node_k].child_left).label];
  int child_order_alt = order[(*tree.nodes[node_k].child_right).label];
  if(child_order < child_order_alt) child_order = child_order_alt;
  if(child_order < N) child_order = N-1;
  assert(child_order < parent_order);

  if( parent_order - child_order > 2 ){

    std::uniform_int_distribution<int> d_swap(child_order+1, parent_order-1);
    int new_order = d_swap(rng);

    //calculate odds
    node_swap_k     = sorted_indices[new_order];
    parent_order    = order[(*tree.nodes[node_swap_k].parent).label];
    child_order     = order[(*tree.nodes[node_swap_k].child_left).label];
    child_order_alt = order[(*tree.nodes[node_swap_k].child_right).label];
    if(child_order < child_order_alt) child_order = child_order_alt;
    if(child_order < N) child_order = N-1;

    if(child_order < k && k < parent_order){

      if(new_order != k){
        sorted_indices[k]         = node_swap_k;
        sorted_indices[new_order] = node_k;
        order[node_k]             = new_order;
        order[node_swap_k]        = k;
      }

    }

  }

}

void
EstimateBranchLengths::SwitchOrder(Tree& tree, int k, std::uniform_real_distribution<double>& dist_unif){

  //Idea:
  //
  //For node_k = sorted_indices[k], look up order of parent and children
  //Then, choose a node with order between children and parent uniformly at random.
  //Check whether chosen node has order of children < k and order of parent > k
  //If so, this is a valid transition - calculate likelihood ratio and accept transition with this probability
  //Otherwise, reject transition immediately
  //
  //Notice that the transition probabilities are symmetric: 
  //Distribution d_swap(child_order+1, parent_order-1); does not change after a transition.
  //
  //TODO: With tips that have nonzero coalescent time, we need to reject any transition proposed with these tips.


  //This update is constant time.
  accept = true;
  int node_k, node_swap_k;
  log_likelihood_ratio = 0.0;

  //check order of parent and order of children
  //choose a random number in this range (not including)
  //swap order with that node

  node_k = sorted_indices[k];

  int parent_order    = order[(*tree.nodes[node_k].parent).label];
  int child_order     = order[(*tree.nodes[node_k].child_left).label];
  int child_order_alt = order[(*tree.nodes[node_k].child_right).label];
  if(child_order < child_order_alt) child_order = child_order_alt;

  if(child_order < N) child_order = N-1;
  assert(child_order < parent_order);

  if( parent_order - child_order > 2 ){

    //log_likelihood_ratio = parent_order - child_order - 2;
    std::uniform_int_distribution<int> d_swap(child_order+1, parent_order-1);
    int new_order = d_swap(rng);

    //calculate odds
    node_swap_k     = sorted_indices[new_order];
    parent_order    = order[(*tree.nodes[node_swap_k].parent).label];
    child_order     = order[(*tree.nodes[node_swap_k].child_left).label];
    child_order_alt = order[(*tree.nodes[node_swap_k].child_right).label];
    if(child_order < child_order_alt) child_order = child_order_alt;
    if(child_order < N) child_order = N-1;

    if(child_order < k && k < parent_order){

      //branch length of node and children change
      delta_tau = coordinates[node_swap_k] - coordinates[node_k];
      //assert(delta_tau >= 0.0);

      child_left_label  = (*tree.nodes[node_k].child_left).label;
      child_right_label = (*tree.nodes[node_k].child_right).label;

      n_num_events           = tree.nodes[node_k].num_events;
      child_left_num_events  = tree.nodes[child_left_label].num_events;
      child_right_num_events = tree.nodes[child_right_label].num_events;

      tb                 = tree.nodes[node_k].branch_length;
      tb_new             = tb - delta_tau;
      tb_child_left      = tree.nodes[child_left_label].branch_length;
      tb_child_left_new  = tb_child_left + delta_tau;
      tb_child_right     = tree.nodes[child_right_label].branch_length;
      tb_child_right_new = tb_child_right + delta_tau;

      //mutation and recombination part
      if(tb == 0.0){
        log_likelihood_ratio  = std::numeric_limits<float>::infinity();
      }else if(tb_new <= 0.0){
        log_likelihood_ratio  = -std::numeric_limits<float>::infinity();
      }else{

        if(tb_child_left == 0.0){
          log_likelihood_ratio  = std::numeric_limits<float>::infinity();
        }else if(tb_child_left_new <= 0.0){
          log_likelihood_ratio  = -std::numeric_limits<float>::infinity(); 
        }else{

          if(tb_child_right == 0.0){
            log_likelihood_ratio  = std::numeric_limits<float>::infinity();
          }else if(tb_child_right_new <= 0.0){
            log_likelihood_ratio  = -std::numeric_limits<float>::infinity();
          }else{

            log_likelihood_ratio += (mut_rate[node_k] - mut_rate[child_left_label] - mut_rate[child_right_label]) * delta_tau;
            log_likelihood_ratio += n_num_events * log(tb_new/tb);
            log_likelihood_ratio += child_right_num_events * log(tb_child_right_new/tb_child_right); 
            log_likelihood_ratio += child_left_num_events * log(tb_child_left_new/tb_child_left); 

            delta_tau  *= -1.0;
            child_left_label  = (*tree.nodes[node_swap_k].child_left).label;
            child_right_label = (*tree.nodes[node_swap_k].child_right).label;

            n_num_events           = tree.nodes[node_swap_k].num_events;
            child_left_num_events  = tree.nodes[child_left_label].num_events;
            child_right_num_events = tree.nodes[child_right_label].num_events;

            tb                 = tree.nodes[node_swap_k].branch_length;
            tb_new             = tb - delta_tau;
            tb_child_left      = tree.nodes[child_left_label].branch_length;
            tb_child_left_new  = tb_child_left + delta_tau;
            tb_child_right     = tree.nodes[child_right_label].branch_length;
            tb_child_right_new = tb_child_right + delta_tau;

            if(tb == 0.0){
              log_likelihood_ratio  = std::numeric_limits<float>::infinity();
            }else if(tb_new <= 0.0){
              log_likelihood_ratio  = -std::numeric_limits<float>::infinity();
            }else{

              if(tb_child_left == 0.0){
                log_likelihood_ratio  = std::numeric_limits<float>::infinity();
              }else if(tb_child_left_new <= 0.0){
                log_likelihood_ratio  = -std::numeric_limits<float>::infinity(); 
              }else{

                if(tb_child_right == 0.0){
                  log_likelihood_ratio  = std::numeric_limits<float>::infinity();
                }else if(tb_child_right_new <= 0.0){
                  log_likelihood_ratio  = -std::numeric_limits<float>::infinity();
                }else{ 
                  log_likelihood_ratio += (mut_rate[node_swap_k] - mut_rate[child_left_label] - mut_rate[child_right_label]) * delta_tau;
                  log_likelihood_ratio += n_num_events * log(tb_new/tb);
                  log_likelihood_ratio += child_right_num_events * log(tb_child_right_new/tb_child_right); 
                  log_likelihood_ratio += child_left_num_events * log(tb_child_left_new/tb_child_left);
                }

              }
            }

          }
        }
      } 
      assert(!std::isnan(log_likelihood_ratio));

      accept = true;
      if(log_likelihood_ratio < 0.0){
        //accept with probability exp(log_likelihood_ratio)
        if(dist_unif(rng) > exp(log_likelihood_ratio)){
          accept = false;
        }
      }

      //update coordinates and sorted_indices
      if(accept && new_order != k){
        count_accept++;
        //order of nodes in k - new_order decreases by one.

        sorted_indices[k]         = node_swap_k;
        sorted_indices[new_order] = node_k;
        order[node_k]             = new_order;
        order[node_swap_k]        = k;

        double tmp_coords                       = coordinates[node_k];
        coordinates[node_k]                     = coordinates[node_swap_k];
        coordinates[node_swap_k]                = tmp_coords;
        update_node1 = node_k;
        update_node2 = node_swap_k;

        //calculate new branch lengths
        tree.nodes[node_k].branch_length                 = coordinates[(*tree.nodes[node_k].parent).label]  - coordinates[node_k];
        assert(tree.nodes[node_k].branch_length >= 0.0); 
        child_left_label                                 = (*tree.nodes[node_k].child_left).label;
        tree.nodes[child_left_label].branch_length       = coordinates[node_k] - coordinates[child_left_label];
        assert(tree.nodes[child_left_label].branch_length >= 0.0);
        child_right_label                                = (*tree.nodes[node_k].child_right).label;
        tree.nodes[child_right_label].branch_length      = coordinates[node_k] - coordinates[child_right_label];
        assert(tree.nodes[child_right_label].branch_length >= 0.0);

        tree.nodes[node_swap_k].branch_length            = coordinates[(*tree.nodes[node_swap_k].parent).label]  - coordinates[node_swap_k];
        assert(tree.nodes[node_swap_k].branch_length >= 0.0); 
        child_left_label                                 = (*tree.nodes[node_swap_k].child_left).label;
        tree.nodes[child_left_label].branch_length       = coordinates[node_swap_k] - coordinates[child_left_label];
        assert(tree.nodes[child_left_label].branch_length >= 0.0);
        child_right_label                                = (*tree.nodes[node_swap_k].child_right).label;
        tree.nodes[child_right_label].branch_length      = coordinates[node_swap_k] - coordinates[child_right_label];
        assert(tree.nodes[child_right_label].branch_length >= 0.0);

      }
    }

  }

  count_proposal++;
}

void
EstimateBranchLengths::ChangeTimeWhilekAncestors(Tree& tree, int k, std::uniform_real_distribution<double>& dist_unif){

  //Idea:
  // We choose a coalescent event, e.g. node_k and propose a new time for it between either
  //1. its parent and children (more difficult because order of coalescent events change)
  //2. the next node decreasing the number of lineages to k-2 and the previous node decreasing the number of lineages to k. (easier)
  //
  //Transition probabilities can by uniform. In both cases, the transition probability will be symmetric in this case.
  //We then need to calculate the likelihood ratio of the model.
  //
  //We reject any change in the coalescent time of tips.

  num_lineages = std::min(N, 2*N-k); 
  assert(num_lineages <= N);
  assert(num_lineages >= 2);

  int node_k = sorted_indices[k];
  k_choose_2 = num_lineages * (num_lineages-1.0)/2.0;
  float k_minus_one_choose_2 = (num_lineages-1.0) * (num_lineages-2.0)/2.0;

  tau_old   = coordinates[sorted_indices[k]] - coordinates[sorted_indices[k-1]]; //time while num_lineages ancestors

  if(k == 2*N-2){

    //choose tau_new according to Gamma(alpha, alpha/tau_old)
    tau_new   = -fast_log(dist_unif(rng)) * tau_old;
    delta_tau = tau_new - tau_old;
    //calculate ratio of proposals
    log_likelihood_ratio = fast_log(tau_old/tau_new) + (tau_new/tau_old - tau_old/tau_new);
    assert(tau_new > 0.0);

    //coalescent prior
    log_likelihood_ratio -= k_choose_2 * delta_tau;

    n = *tree.nodes[node_k].child_left;
    assert(order[n.label] < k);
    tb     = n.branch_length;
    tb_new = tb + delta_tau;
    if(tb == 0.0){
      log_likelihood_ratio  = std::numeric_limits<float>::infinity();
    }else if(tb_new <= 0.0){
      log_likelihood_ratio  = -std::numeric_limits<float>::infinity();
    }else{
      log_likelihood_ratio -= mut_rate[n.label] * delta_tau;
      log_likelihood_ratio += n.num_events * fast_log(tb_new/tb);
    }

    n = *tree.nodes[node_k].child_right;
    assert(order[n.label] < k);
    tb     = n.branch_length;
    tb_new = tb + delta_tau;
    if(tb == 0.0){
      log_likelihood_ratio  = std::numeric_limits<float>::infinity();
    }else if(tb_new <= 0.0){
      log_likelihood_ratio  = -std::numeric_limits<float>::infinity();
    }else{
      log_likelihood_ratio -= mut_rate[n.label] * delta_tau;
      log_likelihood_ratio += n.num_events * fast_log(tb_new/tb);
    }

  }else{

    tau_new = dist_unif(rng) * (coordinates[sorted_indices[k+1]] - coordinates[sorted_indices[k-1]]);
    delta_tau = tau_new - tau_old; //if positive, increases branch lengths of children
    log_likelihood_ratio  = 0.0; //symmetric transition probabilities.
    log_likelihood_ratio += (k_minus_one_choose_2 - k_choose_2) * delta_tau; //coalescent prior

    ////////////////

    child_left_label  = (*tree.nodes[node_k].child_left).label;
    child_right_label = (*tree.nodes[node_k].child_right).label;

    n_num_events           = tree.nodes[node_k].num_events;
    child_left_num_events  = tree.nodes[child_left_label].num_events;
    child_right_num_events = tree.nodes[child_right_label].num_events;

    tb                 = tree.nodes[node_k].branch_length;
    tb_new             = tb - delta_tau;
    tb_child_left      = tree.nodes[child_left_label].branch_length;
    tb_child_left_new  = tb_child_left + delta_tau;
    tb_child_right     = tree.nodes[child_right_label].branch_length;
    tb_child_right_new = tb_child_right + delta_tau;

    //mutation and recombination part
    if(tb == 0.0){
      log_likelihood_ratio  = std::numeric_limits<float>::infinity();
    }else if(tb_new <= 0.0){
      log_likelihood_ratio  = -std::numeric_limits<float>::infinity();
    }else{

      if(tb_child_left == 0.0){
        log_likelihood_ratio  = std::numeric_limits<float>::infinity();
      }else if(tb_child_left_new <= 0.0){
        log_likelihood_ratio  = -std::numeric_limits<float>::infinity(); 
      }else{

        if(tb_child_right == 0.0){
          log_likelihood_ratio  = std::numeric_limits<float>::infinity();
        }else if(tb_child_right_new <= 0.0){
          log_likelihood_ratio  = -std::numeric_limits<float>::infinity();
        }else{

          log_likelihood_ratio += (mut_rate[node_k] - mut_rate[child_left_label] - mut_rate[child_right_label]) * delta_tau;
          log_likelihood_ratio += n_num_events * log(tb_new/tb);
          log_likelihood_ratio += child_right_num_events * log(tb_child_right_new/tb_child_right); 
          log_likelihood_ratio += child_left_num_events * log(tb_child_left_new/tb_child_left); 

        }
      }
    } 
    assert(!std::isnan(log_likelihood_ratio));

  }

  //decide whether to accept proposal or not.
  accept = true;
  if(log_likelihood_ratio < 0.0){
    //accept with probability exp(log_p)
    if(dist_unif(rng) > exp(log_likelihood_ratio)){
      accept = false;
    }
  }

  //update coordinates and sorted_indices
  if(accept){
    count_accept++;

    //order of nodes stays the same as before
    //calculate new branch lengths
    coordinates[node_k]                             += delta_tau;
    update_node1                                     = node_k;
    //calculate new branch lengths 
    if(k != 2*N-2){
      tree.nodes[node_k].branch_length               = coordinates[(*tree.nodes[node_k].parent).label]  - coordinates[node_k];
      if(!(tree.nodes[node_k].branch_length >= 0.0)) std::cerr << coordinates[(*tree.nodes[node_k].parent).label]  << " " << coordinates[node_k] << " " << delta_tau << std::endl;
      assert(tree.nodes[node_k].branch_length >= 0.0); 
    }
    child_left_label                                 = (*tree.nodes[node_k].child_left).label;
    tree.nodes[child_left_label].branch_length       = coordinates[node_k] - coordinates[child_left_label];
    assert(tree.nodes[child_left_label].branch_length >= 0.0);
    child_right_label                                = (*tree.nodes[node_k].child_right).label;
    tree.nodes[child_right_label].branch_length      = coordinates[node_k] - coordinates[child_right_label];
    assert(tree.nodes[child_right_label].branch_length >= 0.0);
  }
  count_proposal++;

}

void
EstimateBranchLengths::ChangeTimeWhilekAncestorsVP(Tree& tree, int k, const std::vector<double>& epoche, const std::vector<double>& coal_rate, std::uniform_real_distribution<double>& dist_unif){

  //Idea:
  //Currently, I am choosing a time while k ancestors and propose a change with transition probabilities ~ Exp(tau_old)
  //This will not work with tips that have a non-zero coalescent time.
  //In this case, we need to choose a coalescent event, e.g. node_k and propose a new time between either
  //1. its parent and children (more difficult)
  //2. the next node decreasing the number of lineages to k-2 and the previous node decreasing the number of lineages to k. (easier)
  //
  //Transition probabilities can by uniform. In both cases, the transition probability will be symmetric in this case.
  //We then need to calculate the likelihood ratio of the model.
  //
  //We reject any change in the coalescent time of tips.

  num_lineages = std::min(N, 2*N-k); 
  assert(num_lineages <= N);
  assert(num_lineages >= 2);

  int node_k = sorted_indices[k];
  k_choose_2 = num_lineages * (num_lineages-1.0)/2.0;
  float k_minus_one_choose_2 = (num_lineages-1.0) * (num_lineages-2.0)/2.0;

  tau_old   = coordinates[sorted_indices[k]] - coordinates[sorted_indices[k-1]]; //time while num_lineages ancestors

  //If k is the root node, I am proposing a new age using an exponential transition probability
  //Otherwise, I propose a transition probability that lies between the next and the previous coalescent event.
  if(k == 2*N-2){

    //choose tau_new according to Gamma(alpha, alpha/tau_old)
    tau_new   = -fast_log(dist_unif(rng)) * tau_old;
    delta_tau = tau_new - tau_old;
    //calculate ratio of proposals
    log_likelihood_ratio = fast_log(tau_old/tau_new) + (tau_new/tau_old - tau_old/tau_new);
    assert(tau_new > 0.0);

    //coalescent prior  
    int ep_begin = 0;
    while(coordinates[sorted_indices[k-1]] >= epoche[ep_begin]){
      ep_begin++;
      if(ep_begin == (int)epoche.size()) break;
    }
    ep_begin--;
    assert(ep_begin > -1);
    assert(coordinates[sorted_indices[k-1]] >= epoche[ep_begin]);
    if( coordinates[sorted_indices[k-1]] >= epoche[ep_begin+1]  ){
      assert(ep_begin == (int) epoche.size() - 1);
    }
    //-k_choose_2 * tau_old + k_choose_2 + tau_new
    int ep;
    double tmp_tau, delta_tmp_tau;
    ep            = ep_begin;
    tmp_tau       = tau_new;
    if(ep < epoche.size() - 1){
      delta_tmp_tau = epoche[ep+1] - coordinates[sorted_indices[k-1]];
      assert(delta_tmp_tau >= 0.0);
      if(delta_tmp_tau <= tmp_tau){
        if(coal_rate[ep] > 0.0){
          log_likelihood_ratio   -= k_choose_2*coal_rate[ep] * delta_tmp_tau; 
        }
        tmp_tau                -= delta_tmp_tau;
        ep++;
        delta_tmp_tau           = epoche[ep+1] - epoche[ep];
        while(tmp_tau > delta_tmp_tau && ep < epoche.size() - 1){
          if(coal_rate[ep] > 0.0){
            log_likelihood_ratio -= k_choose_2*coal_rate[ep] * delta_tmp_tau; 
          }
          tmp_tau              -= delta_tmp_tau;
          ep++;
          delta_tmp_tau         = epoche[ep+1] - epoche[ep];
        }
        assert(tmp_tau >= 0.0);
        if(coal_rate[ep] == 0){
          log_likelihood_ratio  = -std::numeric_limits<float>::infinity();
        }else{
          log_likelihood_ratio -= k_choose_2*coal_rate[ep] * tmp_tau - log(coal_rate[ep]);
        }
      }else{
        if(coal_rate[ep] == 0){
          log_likelihood_ratio = -std::numeric_limits<float>::infinity();
        }else{
          log_likelihood_ratio -= k_choose_2*coal_rate[ep] * tmp_tau - log(coal_rate[ep]);
        }
      }
    }else{
      if(coal_rate[ep] == 0){
        log_likelihood_ratio = -std::numeric_limits<float>::infinity();
      }else{
        log_likelihood_ratio -= k_choose_2*coal_rate[ep] * tmp_tau - log(coal_rate[ep]);
      }
    }

    if(log_likelihood_ratio != -std::numeric_limits<float>::infinity()){

      ep            = ep_begin;
      tmp_tau       = tau_old;
      if(ep < epoche.size() - 1){
        delta_tmp_tau = epoche[ep+1] - coordinates[sorted_indices[k-1]];
        assert(delta_tmp_tau >= 0.0);
        if(delta_tmp_tau <= tmp_tau){
          if(coal_rate[ep] > 0.0){
            log_likelihood_ratio  += k_choose_2*coal_rate[ep] * delta_tmp_tau; 
          }
          tmp_tau                -= delta_tmp_tau;
          ep++;
          delta_tmp_tau           = epoche[ep+1] - epoche[ep];
          while(tmp_tau > delta_tmp_tau && ep < epoche.size() - 1){
            if(coal_rate[ep] > 0.0){
              log_likelihood_ratio += k_choose_2*coal_rate[ep] * delta_tmp_tau; 
            }
            tmp_tau              -= delta_tmp_tau;
            ep++;
            delta_tmp_tau         = epoche[ep+1] - epoche[ep];
          }
          assert(tmp_tau >= 0.0);
          if(coal_rate[ep] == 0){
            log_likelihood_ratio = std::numeric_limits<float>::infinity();
          }else{
            log_likelihood_ratio += k_choose_2*coal_rate[ep] * tmp_tau - log(coal_rate[ep]);
          }
        }else{
          if(coal_rate[ep] == 0){
            log_likelihood_ratio = std::numeric_limits<float>::infinity();
          }else{
            log_likelihood_ratio += k_choose_2*coal_rate[ep] * tmp_tau - log(coal_rate[ep]);
          }
        }
      }else{
        if(coal_rate[ep] == 0){
          log_likelihood_ratio = std::numeric_limits<float>::infinity();
        }else{
          log_likelihood_ratio += k_choose_2*coal_rate[ep] * tmp_tau - log(coal_rate[ep]);
        }
      }

      //rest
      n = *tree.nodes[node_k].child_left;
      assert(order[n.label] < k);
      tb     = n.branch_length;
      tb_new = tb + delta_tau;
      if(tb == 0.0){
        log_likelihood_ratio  = std::numeric_limits<float>::infinity();
      }else if(tb_new <= 0.0){
        log_likelihood_ratio  = -std::numeric_limits<float>::infinity();
      }else{
        log_likelihood_ratio -= mut_rate[n.label] * delta_tau;
        log_likelihood_ratio += n.num_events * fast_log(tb_new/tb);
      }

      n = *tree.nodes[node_k].child_right;
      assert(order[n.label] < k);
      tb     = n.branch_length;
      tb_new = tb + delta_tau;
      if(tb == 0.0){
        log_likelihood_ratio  = std::numeric_limits<float>::infinity();
      }else if(tb_new <= 0.0){
        log_likelihood_ratio  = -std::numeric_limits<float>::infinity();
      }else{
        log_likelihood_ratio -= mut_rate[n.label] * delta_tau;
        log_likelihood_ratio += n.num_events * fast_log(tb_new/tb);
      }

    }

  }else{

    tau_new = dist_unif(rng) * (coordinates[sorted_indices[k+1]] - coordinates[sorted_indices[k-1]]);
    delta_tau = tau_new - tau_old; //if positive, increases branch lengths of children
    log_likelihood_ratio  = 0.0; //symmetric transition probabilities.

    //coalescent prior  
    int ep_begin = 0;
    while(coordinates[sorted_indices[k-1]] >= epoche[ep_begin]){
      ep_begin++;
      if(ep_begin == (int)epoche.size()) break;
    }
    ep_begin--;
    assert(ep_begin > -1);
    assert(coordinates[sorted_indices[k-1]] >= epoche[ep_begin]);
    if( coordinates[sorted_indices[k-1]] >= epoche[ep_begin+1]  ){
      assert(ep_begin == (int) epoche.size() - 1);
    }


    //-k_choose_2 * tau_old + k_choose_2 + tau_new
    int ep;
    double tmp_tau, delta_tmp_tau;
    ep            = ep_begin;
    tmp_tau       = tau_new;

    int k_max = k+2;
    int k_tmp = k;
    int num_lineages_tmp = num_lineages;
    float k_choose_2_tmp = k_choose_2;
    while(k_tmp < k_max){
      if(ep < epoche.size() - 1){

        if(k_tmp > k){
          tmp_tau = coordinates[sorted_indices[k_tmp]] - coordinates[sorted_indices[k_tmp-1]] - delta_tau;
          delta_tmp_tau = epoche[ep+1] - (coordinates[sorted_indices[k_tmp-1]] + delta_tau);
          //update k_choose_2
          k_choose_2_tmp *= (num_lineages_tmp - 2.0)/num_lineages_tmp;
          num_lineages_tmp--;
          assert(num_lineages_tmp > 1);
        }else{
          delta_tmp_tau = epoche[ep+1] - coordinates[sorted_indices[k_tmp-1]];
        }

        assert(delta_tmp_tau >= 0.0);
        if(delta_tmp_tau <= tmp_tau){
          if(coal_rate[ep] > 0.0){
            log_likelihood_ratio -= k_choose_2_tmp*coal_rate[ep] * delta_tmp_tau; 
          }
          tmp_tau                -= delta_tmp_tau;
          ep++;
          delta_tmp_tau           = epoche[ep+1] - epoche[ep];
          while(tmp_tau > delta_tmp_tau && ep < epoche.size() - 1){
            if(coal_rate[ep] > 0.0){
              log_likelihood_ratio -= k_choose_2_tmp*coal_rate[ep] * delta_tmp_tau; 
            }
            tmp_tau              -= delta_tmp_tau;
            ep++;
            delta_tmp_tau         = epoche[ep+1] - epoche[ep];
          }
          assert(tmp_tau >= 0.0);
          if(coal_rate[ep] == 0){
            log_likelihood_ratio  = -std::numeric_limits<float>::infinity();
          }else{
            log_likelihood_ratio -= k_choose_2_tmp*coal_rate[ep] * tmp_tau - log(coal_rate[ep]);
          }
        }else{
          if(coal_rate[ep] == 0){
            log_likelihood_ratio = -std::numeric_limits<float>::infinity();
          }else{
            log_likelihood_ratio -= k_choose_2_tmp*coal_rate[ep] * tmp_tau - log(coal_rate[ep]);
          }
        }

      }else{
        if(coal_rate[ep] == 0){
          log_likelihood_ratio = -std::numeric_limits<float>::infinity();
        }else{
          log_likelihood_ratio -= k_choose_2_tmp*coal_rate[ep] * tmp_tau - log(coal_rate[ep]);
        }
      }

      k_tmp++;
    }

    if(log_likelihood_ratio != -std::numeric_limits<float>::infinity()){

      ep            = ep_begin;
      tmp_tau       = tau_old;

      k_tmp = k;
      k_choose_2_tmp = k_choose_2;
      num_lineages_tmp = num_lineages;
      while(k_tmp < k_max){

        if(ep < epoche.size() - 1){

          if(k_tmp > k){
            tmp_tau = coordinates[sorted_indices[k_tmp]] - coordinates[sorted_indices[k_tmp-1]];
            delta_tmp_tau = epoche[ep+1] - coordinates[sorted_indices[k_tmp-1]];
            //update k_choose_2
            k_choose_2_tmp *= (num_lineages_tmp - 2.0)/num_lineages_tmp;
            num_lineages_tmp--;
          }else{
            delta_tmp_tau = epoche[ep+1] - coordinates[sorted_indices[k_tmp-1]];
          }

          assert(delta_tmp_tau >= 0.0);
          if(delta_tmp_tau <= tmp_tau){
            if(coal_rate[ep] > 0.0){
              log_likelihood_ratio  += k_choose_2_tmp*coal_rate[ep] * delta_tmp_tau; 
            }
            tmp_tau                -= delta_tmp_tau;
            ep++;
            delta_tmp_tau           = epoche[ep+1] - epoche[ep];
            while(tmp_tau > delta_tmp_tau && ep < epoche.size() - 1){
              if(coal_rate[ep] > 0.0){
                log_likelihood_ratio += k_choose_2_tmp*coal_rate[ep] * delta_tmp_tau; 
              }
              tmp_tau              -= delta_tmp_tau;
              ep++;
              delta_tmp_tau         = epoche[ep+1] - epoche[ep];
            }
            assert(tmp_tau >= 0.0);
            if(coal_rate[ep] == 0){
              log_likelihood_ratio = std::numeric_limits<float>::infinity();
            }else{
              log_likelihood_ratio += k_choose_2_tmp*coal_rate[ep] * tmp_tau - log(coal_rate[ep]);
            }

          }else{
            if(coal_rate[ep] == 0){
              log_likelihood_ratio = std::numeric_limits<float>::infinity();
            }else{
              log_likelihood_ratio += k_choose_2_tmp*coal_rate[ep] * tmp_tau - log(coal_rate[ep]);
            }
          }

        }else{
          if(coal_rate[ep] == 0){
            log_likelihood_ratio = std::numeric_limits<float>::infinity();
          }else{
            log_likelihood_ratio += k_choose_2_tmp*coal_rate[ep] * tmp_tau - log(coal_rate[ep]);
          }
        }

        k_tmp++;
      }



      ////////////////

      child_left_label  = (*tree.nodes[node_k].child_left).label;
      child_right_label = (*tree.nodes[node_k].child_right).label;

      n_num_events           = tree.nodes[node_k].num_events;
      child_left_num_events  = tree.nodes[child_left_label].num_events;
      child_right_num_events = tree.nodes[child_right_label].num_events;

      tb                 = tree.nodes[node_k].branch_length;
      tb_new             = tb - delta_tau;
      tb_child_left      = tree.nodes[child_left_label].branch_length;
      tb_child_left_new  = tb_child_left + delta_tau;
      tb_child_right     = tree.nodes[child_right_label].branch_length;
      tb_child_right_new = tb_child_right + delta_tau;

      //mutation and recombination part
      if(tb == 0.0){
        log_likelihood_ratio  = std::numeric_limits<float>::infinity();
      }else if(tb_new <= 0.0){
        log_likelihood_ratio  = -std::numeric_limits<float>::infinity();
      }else{

        if(tb_child_left == 0.0){
          log_likelihood_ratio  = std::numeric_limits<float>::infinity();
        }else if(tb_child_left_new <= 0.0){
          log_likelihood_ratio  = -std::numeric_limits<float>::infinity(); 
        }else{

          if(tb_child_right == 0.0){
            log_likelihood_ratio  = std::numeric_limits<float>::infinity();
          }else if(tb_child_right_new <= 0.0){
            log_likelihood_ratio  = -std::numeric_limits<float>::infinity();
          }else{

            log_likelihood_ratio += (mut_rate[node_k] - mut_rate[child_left_label] - mut_rate[child_right_label]) * delta_tau;
            log_likelihood_ratio += n_num_events * log(tb_new/tb);
            log_likelihood_ratio += child_right_num_events * log(tb_child_right_new/tb_child_right); 
            log_likelihood_ratio += child_left_num_events * log(tb_child_left_new/tb_child_left); 

          }
        }
      } 
      assert(!std::isnan(log_likelihood_ratio));

    }

  }

  //decide whether to accept proposal or not.
  accept = true;
  if(log_likelihood_ratio < 0.0){
    //accept with probability exp(log_p)
    if(dist_unif(rng) > exp(log_likelihood_ratio)){
      accept = false;
    }
  }

  //update coordinates and sorted_indices
  if(accept){
    count_accept++;

    //order of nodes stays the same as before
    //calculate new branch lengths
    coordinates[node_k]                             += delta_tau;
    update_node1                                     = node_k;
    //calculate new branch lengths 
    if(k != 2*N-2){
      tree.nodes[node_k].branch_length               = coordinates[(*tree.nodes[node_k].parent).label]  - coordinates[node_k];
      assert(tree.nodes[node_k].branch_length >= 0.0); 
    }
    child_left_label                                 = (*tree.nodes[node_k].child_left).label;
    tree.nodes[child_left_label].branch_length       = coordinates[node_k] - coordinates[child_left_label];
    assert(tree.nodes[child_left_label].branch_length >= 0.0);
    child_right_label                                = (*tree.nodes[node_k].child_right).label;
    tree.nodes[child_right_label].branch_length      = coordinates[node_k] - coordinates[child_right_label];
    assert(tree.nodes[child_right_label].branch_length >= 0.0);

  }
  count_proposal++;

}



///////////////////////////////////////////


void
EstimateBranchLengths::GetCoordinates(Node& n, std::vector<double>& coords){

  if(n.child_left != NULL){
    GetCoordinates(*n.child_left, coords);
    GetCoordinates(*n.child_right, coords);

    coords[n.label] = coords[(*n.child_left).label] + (*n.child_left).branch_length;

  }else{
    coords[n.label] = 0.0;
  }

}

void
EstimateBranchLengths::MCMC(const Data& data, Tree& tree, const int seed){

  count_accept = 0;
  count_proposal = 0;
  int delta = std::max(data.N/10.0, 10.0);
  convergence_threshold = 10.0/Ne; //approximately x generations

  //Random number generators
  float uniform_rng;
  rng.seed(seed);
  std::uniform_real_distribution<double> dist_unif(0,1);
  std::uniform_int_distribution<int> dist_k(N,N_total-1);
  std::uniform_int_distribution<int> dist_switch(N,N_total-2);

  root = N_total - 1;

  ////////// Initialize MCMC ///////////

  //Initialize MCMC using coalescent prior
  InitializeMCMC(data, tree); 

  //Randomly switch around order of coalescences
  for(int j = 0; j < (int) data.N * data.N; j++){
    RandomSwitchOrder(tree, dist_switch(rng), dist_unif);
  }   
  InitializeBranchLengths(tree);


  /////////////// Start MCMC ///////////////

  //transient
  count = 0;
  for(; count < 100*delta; count++){

    //Either switch order of coalescent event or extent time while k ancestors 
    uniform_rng = dist_unif(rng);
    if(uniform_rng < 0.5){
      SwitchOrder(tree, dist_switch(rng), dist_unif);
    }else{ 
      ChangeTimeWhilekAncestors(tree, dist_k(rng), dist_unif);
    }

  }

  //Now start estimating branch lenghts. We store the average of coalescent ages in avg calculate branch lengths from here.
  //UpdateAvg is used to update avg.
  //Coalescent ages of the tree are stored in coordinates (this is updated in SwitchOrder and ChangeTimeWhilekAncestors)

  avg              = coordinates;
  last_coordinates = coordinates;
  last_update.resize(N_total);
  std::fill(last_update.begin(), last_update.end(), 1);
  count = 1;

  int num_iterations = 0;
  bool is_count_threshold = false;
  std::vector<int> count_proposals(N_total-N, 0);
  is_avg_increasing = false;
  while(!is_avg_increasing){

    do{

      count++;

      //Either switch order of coalescent event or extent time while k ancestors 
      uniform_rng = dist_unif(rng);
      if(uniform_rng < 0.5){
        SwitchOrder(tree, dist_switch(rng), dist_unif);
        UpdateAvg(tree);
      }else{ 
        int k_candidate = dist_k(rng);
        count_proposals[k_candidate-N]++;
        ChangeTimeWhilekAncestors(tree, k_candidate, dist_unif);
        count++;
        UpdateAvg(tree);
      }

    }while(count % delta != 0 );

    num_iterations++;

    //MCMC is allowed to terminate if is_count_threshold == true and is_avg_increasing == true.
    //At first, both are set to false, and is_count_threshold will be set to true first (once conditions are met)
    //then is_avg_increasing will be set to true eventually.

    if(!is_count_threshold){
      //assert(is_avg_increasing);
      is_avg_increasing = true;
      for(std::vector<int>::iterator it_count = count_proposals.begin(); it_count != count_proposals.end(); it_count++){
        if(*it_count < 100){
          is_avg_increasing = false;
          break;
        }
      }
      if(is_avg_increasing) is_count_threshold = true;
    }

    if(is_avg_increasing){
      //update all nodes in avg 
      it_avg = std::next(avg.begin(), N);
      it_coords = std::next(coordinates.begin(), N);
      it_last_update = std::next(last_update.begin(), N);
      it_last_coords = std::next(last_coordinates.begin(), N);
      //update avg
      for(int ell = N; ell < N_total; ell++){
        *it_avg  += ((count - *it_last_update) * (*it_last_coords - *it_avg))/count;
        *it_last_update = count;
        *it_last_coords = *it_coords;
        it_avg++;
        it_coords++;
        it_last_update++;
        it_last_coords++;
      }
      //check if coalescent ages are non-decreasing in avg
      int ell = N;
      is_avg_increasing = true;
      for(it_avg = std::next(avg.begin(),N); it_avg != avg.end(); it_avg++){
        if(ell < root){
          if(*it_avg > avg[(*tree.nodes[ell].parent).label]) is_avg_increasing = false;
        }
        ell++;
      }
    }

  }

  //////////// Caluclate branch lengths from avg ////////////

  //don't need to update avg again because I am guaranteed to finish with having updated all nodes
  for(std::vector<Node>::iterator it_n = tree.nodes.begin(); it_n != std::prev(tree.nodes.end(),1); it_n++){
    (*it_n).branch_length = ((double) Ne) * (avg[(*(*it_n).parent).label] - avg[(*it_n).label]);
  }

  /*
     for(std::vector<Node>::iterator it_n = tree.nodes.begin(); it_n != std::prev(tree.nodes.end(),1); it_n++){
     (*it_n).branch_length = ((double) Ne) * (*it_n).branch_length;
     }
     */

}  

void
EstimateBranchLengths::MCMCVariablePopulationSize(const Data& data, Tree& tree, const std::vector<double>& epoche, std::vector<double>& coal_rate, const int seed){

  count_accept = 0;
  count_proposal = 0;

  float uniform_rng;
  rng.seed(seed);
  std::uniform_real_distribution<double> dist_unif(0,1);
  std::uniform_int_distribution<int> dist_k(N,N_total-1);
  std::uniform_int_distribution<int> dist_switch(N,N_total-2);

  convergence_threshold = 100.0/Ne; //approximately x generations

  int delta = std::max(data.N/10.0, 10.0);
  //int delta = 10;
  root = N_total - 1;

  InitializeMCMC(data, tree); //Initialize using coalescent prior 

  for(std::vector<Node>::iterator it_node = tree.nodes.begin(); it_node != tree.nodes.end(); it_node++){
    (*it_node).branch_length /= data.Ne;
  }

  coordinates.resize(N_total);
  GetCoordinates(tree.nodes[root], coordinates);
  //avg   = coordinates;

  std::size_t m1(0);
  std::generate(std::begin(sorted_indices) + N, std::end(sorted_indices), [&]{ return m1++; });
  std::sort(std::begin(sorted_indices) + N, std::end(sorted_indices), [&](int i1, int i2) { return coordinates[i1 + N] < coordinates[i2 + N]; } );
  for(int i = 0; i < N; i++){
    sorted_indices[i]  = i;
  }
  for(int i = N; i < N_total; i++){
    sorted_indices[i] += N;
  }

  //obtain order of coalescent events
  std::fill(order.begin(), order.end(), 0);
  std::size_t m2(0);
  std::generate(std::begin(order) + N, std::end(order), [&]{ return m2++; });
  std::sort(std::begin(order) + N, std::end(order), [&](int i1, int i2) { return sorted_indices[i1 + N] < sorted_indices[i2 + N]; } );

  for(int i = 0; i < N; i++){
    order[i]  = i;
  }
  for(int i = N; i < N_total; i++){
    order[i] += N;
  }

  //This is for when branch lengths are zero, because then the above does not guarantee parents to be above children
  bool is_topology_violated = true;
  while(is_topology_violated){
    is_topology_violated = false;
    for(int i = N; i < N_total; i++){
      int node_k = sorted_indices[i];
      if( order[(*tree.nodes[node_k].child_left).label] > order[node_k] ){
        int tmp_order = order[node_k];
        order[node_k] = order[(*tree.nodes[node_k].child_left).label];
        order[(*tree.nodes[node_k].child_left).label] = tmp_order;
        sorted_indices[order[node_k]] = node_k;
        sorted_indices[tmp_order] = (*tree.nodes[node_k].child_left).label;
        is_topology_violated = true;
      }
      if( order[(*tree.nodes[node_k].child_right).label] > order[node_k] ){
        int tmp_order = order[node_k];
        order[node_k] = order[(*tree.nodes[node_k].child_right).label];
        order[(*tree.nodes[node_k].child_right).label] = tmp_order;
        sorted_indices[order[node_k]] = node_k;
        sorted_indices[tmp_order] = (*tree.nodes[node_k].child_right).label;
        is_topology_violated = true;
      }
    }
  }

  //std::cerr << "Number of branches with no mutations: branches_with_no_mutations " << branches_with_no_mutations.size() << std::endl;

  ////////////////// Transient /////////////////

  count = 0;
  for(; count < 100*delta; count++){

    //Either switch order of coalescent event or extent time while k ancestors 
    uniform_rng = dist_unif(rng);
    if(uniform_rng < 0.8){
      SwitchOrder(tree, dist_switch(rng), dist_unif);
    }else{ 
      ChangeTimeWhilekAncestorsVP(tree, dist_k(rng), epoche, coal_rate, dist_unif);
    }

  }

  avg              = coordinates;
  last_coordinates = coordinates;
  last_update.resize(N_total);
  std::fill(last_update.begin(), last_update.end(), 1);
  count = 1;

  int num_iterations = 0;
  bool is_count_threshold = false;
  std::vector<int> count_proposals(N_total-N, 0);
  is_avg_increasing = false;
  while(!is_avg_increasing){

    do{

      count++;

      //Either switch order of coalescent event or extent time while k ancestors 
      uniform_rng = dist_unif(rng);
      if(uniform_rng < 0.8){
        SwitchOrder(tree, dist_switch(rng), dist_unif);
        UpdateAvg(tree);
      }else{ 
        int k_candidate = dist_k(rng);
        count_proposals[k_candidate-N]++;
        ChangeTimeWhilekAncestorsVP(tree, k_candidate, epoche, coal_rate, dist_unif);
        count++;
        UpdateAvg(tree);
      }

    }while(count % delta != 0 );

    num_iterations++;

    //assert(is_avg_increasing);
    if(!is_count_threshold){
      is_avg_increasing = true;
      for(std::vector<int>::iterator it_count = count_proposals.begin(); it_count != count_proposals.end(); it_count++){
        if(*it_count < N){
          is_avg_increasing = false;
          break;
        }
      }
      if(is_avg_increasing) is_count_threshold = true;
    }

    if(is_avg_increasing){
      //update all nodes in avg 
      it_avg = std::next(avg.begin(), N);
      it_coords = std::next(coordinates.begin(), N);
      it_last_update = std::next(last_update.begin(), N);
      it_last_coords = std::next(last_coordinates.begin(), N);
      //update avg
      for(int ell = N; ell < N_total; ell++){
        *it_avg  += ((count - *it_last_update) * (*it_last_coords - *it_avg))/count;
        *it_last_update = count;
        *it_last_coords = *it_coords;
        it_avg++;
        it_coords++;
        it_last_update++;
        it_last_coords++;
      }

      //calculate max diff to previous avg coalescent times
      int ell = N;
      is_avg_increasing = true;
      for(it_avg = std::next(avg.begin(),N); it_avg != avg.end(); it_avg++){
        if(ell < root){
          if(*it_avg > avg[(*tree.nodes[ell].parent).label]) is_avg_increasing = false;
        }
        ell++;
      }
    }

  }

  for(std::vector<Node>::iterator it_n = tree.nodes.begin(); it_n != std::prev(tree.nodes.end(),1); it_n++){
    (*it_n).branch_length = ((double) Ne) * (avg[(*(*it_n).parent).label] - avg[(*it_n).label]);
  }


  /*
     for(std::vector<Node>::iterator it_n = tree.nodes.begin(); it_n != std::prev(tree.nodes.end(),1); it_n++){
     (*it_n).branch_length = ((double) Ne) * (*it_n).branch_length;
     }
     */


}  

