/*
 * master.cpp
 *
 *  Created on: Oct 29, 2020
 *      Author: geunyeongbyeon
 */

// #define M_BUILD_DEBUG
// #define M_SOLVE_DEBUG

#include "master.h"
#include "macro.h"
#include "hpp.h"

Master::Master()
{
    env_ = new IloEnv,
    m_ = IloModel(*env_);
    vars_ = masterVars(env_);
    constrs_ = masterConstrs(env_);
    xVals_ = IloNumArray (*env_);
    cx_ = IloNumArray (*env_);
    cy_ = IloNumArray (*env_);
    cy_expr_ = IloExpr (*env_);
    dy_ = IloNumArray (*env_);
    dy_expr_ = IloExpr (*env_);
}

/* copy constructor */
Master::Master(const Master & rhs) :
env_(rhs.env_),
m_(rhs.m_),
vars_(rhs.vars_),
constrs_(rhs.constrs_),
xVals_(rhs.xVals_),
cx_(rhs.cx_),
cy_(rhs.cy_),
cy_expr_(rhs.cy_expr_),
dy_(rhs.dy_),
dy_expr_(rhs.dy_expr_)
{
    /* nothing to do */
}

Master::~Master()
{
    xVals_.end();
    cx_.end();
    dy_.end();
    dy_expr_.end();
    cy_.end();
    cy_expr_.end();

    vars_.x.end();
    vars_.y.end();
    
    constrs_.l.end();
    constrs_.f.end();
    constrs_.yLBs.end();
    constrs_.yUBs.end();
    constrs_.xBds.end();
    constrs_.optCuts.end();
    constrs_.feasCuts.end();

    cplex_.end();
    m_.end();
    env_->end();

    delete env_;
}

void Master::loadProblem (Data &data) {

    n_l_ = data.n_l_;
    n_f_ = data.n_f_;
    m_l_ = data.m_l_;
    m_f_ = data.m_f_;

    xlb_ = data.xlb_;
    xub_ = data.xub_;
    ind_col_ = data.ind_col_;
    scale_col_ = data.scale_col_;
    map_varind_to_lvarind_ = data.map_varind_to_lvarind_;
    lObj_constant_ = data.lObj_constant_;

    is_integer_ = data.is_integer_;
    llObj_ = data.llObj_;
    
    ylb_cnt_ = data.ylb_cnt_;
    ylb_ind_ = data.ylb_ind_;
    ylb_coef_ = data.ylb_coef_;
    yub_cnt_ = data.yub_cnt_;
    yub_ind_ = data.yub_ind_;
    yub_coef_ = data.yub_coef_;
    
    fObj_ = data.fObj_;
    lfObj_ = data.lfObj_;
    
    fC_rhs_ = data.fC_rhs_;
    fC_fV_cnt_ = data.fC_fV_cnt_;
    fC_lV_cnt_ = data.fC_lV_cnt_;
    fC_fV_coef_= data.fC_fV_coef_;
    fC_fV_ind_ = data.fC_fV_ind_;
    fC_lV_coef_ = data.fC_lV_coef_;
    fC_lV_ind_ = data.fC_lV_ind_;

    lC_rhs_ = data.lC_rhs_;
    lC_fV_cnt_ = data.lC_fV_cnt_;
    lC_lV_cnt_ = data.lC_lV_cnt_;
    lC_fV_coef_ = data.lC_fV_coef_;
    lC_fV_ind_ = data.lC_fV_ind_;
    lC_lV_coef_ = data.lC_lV_coef_;
    lC_lV_ind_ = data.lC_lV_ind_;
}

/* master problem do not need to have y variables, leader and follower constraints, and t >= cy 
 * having them in the model does not compromise optimality 
 * current version includes them in the master to accelerate the method
 */
void Master::createProblem () {

    int i, j;
    
#ifdef M_BUILD_DEBUG
    /* tic */
    auto start_t = chrono::system_clock::now();
#endif

    addxVars();
    addyVars();
    addtVar();

#ifdef M_BUILD_DEBUG
    /* toc */
    ticToc_ = (chrono::system_clock::now() - start_t);
    cout << "time in adding vars to master: " << ticToc_.count() << endl;
    /* tic */
    start_t = chrono::system_clock::now();
#endif

    /* add leader constraints: Gx + Gy >= h */
    addlConstr();

#ifdef M_BUILD_DEBUG
    /* toc */
    ticToc_ = (chrono::system_clock::now() - start_t);
    cout << "time in addlConstr to master: " << ticToc_.count() << endl;    
    /* tic*/
    start_t = chrono::system_clock::now();
#endif

    /* add follower constraints with x: Ax + By >= b */
    addfConstr();

#ifdef M_BUILD_DEBUG
    /* toc */
    ticToc_ = (chrono::system_clock::now() - start_t);
    cout << "time in addfConstr to mater: " << ticToc_.count() << endl;
    /* tic*/
    start_t = chrono::system_clock::now();
#endif

    /* add bound constraints on x */
    addxBdsConstr();

#ifdef M_BUILD_DEBUG
    /* toc */
    ticToc_ = (chrono::system_clock::now() - start_t);
    cout << "time in addxBdsConstr to master: " << ticToc_.count() << endl;
    /* tic */
    start_t = chrono::system_clock::now();
#endif

    /* add bound constraints on y */
    addyBdsConstr();

#ifdef M_BUILD_DEBUG
    /* toc */
    ticToc_ = (chrono::system_clock::now() - start_t);
    cout << "time in adding yBdconstrs to master: " << ticToc_.count() << endl;
    /* tic */
    start_t = chrono::system_clock::now();
#endif

    /* add follower objective constraint: dy <= 0 */
    // addfObjConstr();

    /* define cplex */
    cplex_ = IloCplex(m_);

#ifdef M_BUILD_DEBUG
    /* toc */
    ticToc_ = (chrono::system_clock::now() - start_t);
    cout << "time in defining master cplex: " << ticToc_.count() << endl;
    /* tic */
    start_t = chrono::system_clock::now();
#endif

    /* set objective function: minimized cx + t */
    m_.add(IloMinimize(*env_));
    for (int i = 0; i < n_l_; i++) {
        cx_.add(llObj_[i]);
    }
    cplex_.getObjective().setLinearCoef(vars_.t, 1);
    cplex_.getObjective().setLinearCoefs(vars_.x, cx_);
    cplex_.getObjective().setConstant(lObj_constant_);

#ifdef M_BUILD_DEBUG
    /* toc */
    ticToc_ = (chrono::system_clock::now() - start_t);
    cout << "time in setting master obj: " << ticToc_.count() << endl;
    /* tic */
    start_t = chrono::system_clock::now();
#endif

    /* set cplex parameters */
    // if (!sett.is_callback)
        // cplex_.setOut(env_->getNullStream());

    // cplex_.setParam(IloCplex::Param::Preprocessing::Reduce, 1);
    // cplex_.setParam(IloCplex::Param::Preprocessing::Linear, CPX_OFF);
    cplex_.setParam(IloCplex::Param::MIP::Tolerances::Integrality, 1e-9);
//    master.cplex.setParam(IloCplex::Param::MIP::Tolerances::Linearization, 1e-9);
    cplex_.setParam(IloCplex::Param::Simplex::Tolerances::Feasibility, 1e-9);

    cplex_.setParam(IloCplex::Param::WorkMem, 12000);
    cplex_.setParam(IloCplex::Param::MIP::Strategy::File, 3);
    // cplex_.setParam(IloCplex::Param::MIP::Limits::Nodes, 0);
    // cplex_.setParam(IloCplex::Param::MIP::Strategy::VariableSelect, CPX_VARSEL_STRONG);

    // cplex_.setParam(IloCplex::Param::Threads, 1);
//    master.cplex.setParam(IloCplex::Param::Preprocessing::Presolve, IloFalse);

    // cplex_.setParam(IloCplex::Param::Parallel, CPX_PARALLEL_OPPORTUNISTIC);
    // cplex_.setParam(IloCplex::Param::MIP::Strategy::HeuristicFreq, 200);//100); //-1);
    // cplex_.setParam(IloCplex::Param::MIP::Strategy::VariableSelect, CPX_VARSEL_STRONG);
    // cplex_.setParam(IloCplex::Param::MIP::Strategy::NodeSelect, CPX_NODESEL_BESTEST);
    // cplex_.setParam(IloCplex::Param::MIP::Strategy::Probe, 3);
    // cplex_.setParam(IloCplex::Param::MIP::Strategy::PresolveNode, -1);
    // cplex_.setParam(IloCplex::Param::RootAlgorithm, CPX_ALG_BARRIER);


    /* set branching priority */
    /* if x appears in follower problem, increase priority */
    vector<int> priority(n_l_, 1);
    for (i = 0; i < m_f_; i++) {
        for (j = 0; j < fC_lV_cnt_[i]; j++) {
            priority[fC_lV_ind_[i][j]] = 2;
        }
    }
    for (i = 0; i < n_l_; i++)
        cplex_.setPriority(vars_.x[i], priority[i]);

#ifdef M_BUILD_DEBUG
    /* toc */
    ticToc_ = (chrono::system_clock::now() - start_t);
    cout << "time in setting branching priority of master: " << ticToc_.count() << endl;
    /* tic */
    start_t = chrono::system_clock::now();
#endif

    /* TO ACCELERATE, NOT NECESSARY: add y vars to master */
    for (i = 0; i < n_f_; i++) {
        cy_.add(lfObj_[i]);
    }
    cy_expr_.setLinearCoefs(vars_.y, cy_);
    m_.add(IloRange(*env_, 0, vars_.t - cy_expr_, IloInfinity));

#ifdef M_BUILD_DEBUG
    /* toc */
    ticToc_ = (chrono::system_clock::now() - start_t);
    cout << "time in add yobj constr to master: " << ticToc_.count() << endl;
#endif

    for (i = 0; i < n_f_; i++)
        dy_.add(fObj_[i]);
    dy_expr_.setLinearCoefs(vars_.y, dy_);

//    cplex_.exportModel("master.lp");

}

int Master::solve() {
    auto start_t = chrono::system_clock::now();

    if (!cplex_.solve()) {
        env_->error() << "Failed to optimize master." << endl;
        throw (-1);
    }

    if (cplex_.getStatus() != IloAlgorithm::Status::Optimal
        && cplex_.getStatus() != IloAlgorithm::Status::Unbounded ) {
        cout << "master is terminated w/ status " << cplex_.getStatus() << endl;
        return 1;
    }

    if (cplex_.getStatus() == IloAlgorithm::Status::Unbounded)
        objVal_ = -3e+6;
    else
        objVal_ = cplex_.getObjValue();

    cplex_.getValues(xVals_, vars_.x);
    tVal_ = cplex_.getValue(vars_.t);

    ticToc_ = (chrono::system_clock::now() - start_t);
    cout << "Master obj: " << objVal_ << endl;
    cout << "time in solving master: " << ticToc_.count() << endl;

    return 0;
}

void Master::solveCallback(Follower &follower, FollowerMC &followerMC, LeaderFollower &leaderFollower, Data &data){

    auto start_t = chrono::system_clock::now();
    
    /* create hpp problem:
     * min c_x x + c_y y : G_x x + G_y y >= h   (l)
     *                     Ax + By >= b         (f)
     *                     x bounds, y bounds   (xBds, yLbs, yUbs)
     */
    Hpp hpp;
    hpp.loadProblem(data);
    hpp.createProblem();

    Hpp hpp_lp;
    if (use_heuristic_cb_)
    {
        hpp_lp.loadProblem(data);
        hpp_lp.createProblem(true);
    }

    /* calculate upper bound of follower obj val */
    if (hpp.solvefUb())
        follower.setfUb(hpp.getfUb());
    else {
        env_->error() << "failed to find fUB." << endl;
        throw(-1);
    }

    // if (hpp.solvefLb())
    // {
    //     follower.setLB(floor(hpp.getfLb()));
    // }

    double blb = 0;
    double bub = 0;
    bool is_ended = false;
    lazyData_ = LazyData (n_l_, n_f_, *env_);

    try {
        
        /* lazyCBBenders.h */
        BendersLazyCallbackI * cb_benders = new BendersLazyCallbackI(*env_, follower, leaderFollower, vars_.x, vars_.t, dy_expr_, lazyData_);
        /* lazyCBBendersMC.h */
        BendersLazyCallbackMCI * cb_benders_mc = new BendersLazyCallbackMCI(*env_, followerMC, leaderFollower, vars_.x, vars_.t, dy_expr_, lazyData_);
        /* heuristicCBIncumbentUpdate.h */
        incumbentUpdateCallbackI * cb_incumbent_update = new incumbentUpdateCallbackI(*env_, vars_.x, lazyData_);
        /* usercutCBBendersMC.h */
        // cplex_.use(BendersUserCallbackMC(*env_, followerMC, leaderFollower, vars_.x, vars_.t, dy_expr_, lazyData_));
        /* usercutCBfUB.h */
        UserCallbackfUBI * cb_fub = new UserCallbackfUBI(*env_, vars_.x, vars_.y, dy_expr_, lazyData_, follower);
        /* usercutCBfUBHeuristic.h */
        // cout << "use hpp with integrality conditions";
        // UserCallbackfUBHeuristicI * cb_fub_nu = new UserCallbackfUBHeuristicI(*env_, vars_.x, vars_.y, dy_expr_, lazyData_, follower, hpp, data);
        /* use relaxed hpp for heuristic */
        UserCallbackfUBHeuristicI * cb_fub_heuristic = new UserCallbackfUBHeuristicI(*env_, vars_.x, vars_.y, dy_expr_, lazyData_, follower, hpp_lp, data);
        /* usercutCBfUBhpp.h */
        // cplex_.use(BendersUserCallbackHpp(*env_, vars_.x, vars_.y, dy_expr_, lazyData_, hpp));
        /* branchCB.h */
        // cplex_.use(branchCallback(*env_, vars_.x, vars_.y, vars_.t, dy_expr_, lazyData_, &blb, &bub, &is_ended, follower, leaderFollower));
        /* nodeCB.h */
        // cplex_.use(nodeSelectCallback(*env_, &blb, &bub, &is_ended, follower.getLB(), follower.getbigM() - follower.getLB()));
        /* nodeCBfObj.h */
        bool node_type = false;
        IloInt64 node_id = -1;
        nodeSelectCallbackfObjI * cb_node_select = new nodeSelectCallbackfObjI(*env_, &node_type, &node_id);
        /* branchCBfObj.h */
        branchCallbackfObjI * cb_branch = new branchCallbackfObjI(*env_, vars_.x, vars_.y, dy_expr_, lazyData_, follower, &node_type, &node_id);
        
        /* callbacks to use */
        if (use_numerically_stable_cut_cb_)
            cplex_.use(cb_benders);
        else cplex_.use(cb_benders_mc);
            
        cplex_.use(cb_incumbent_update);
        if (use_heuristic_cb_)
        {
            cplex_.use(cb_fub_heuristic);
            cplex_.use(cb_node_select);
            cplex_.use(cb_branch);
        
            // heuristic timelimit
            setTimeLimit(heuristic_time_limit_);
            if (!cplex_.solve()) {
                env_->error() << "Failed to optimize master." << endl;
                throw (-1);
            }

            /* record heuristic statistics */
            fub_gap_ = cplex_.getMIPRelativeGap();
            num_local_cuts_added_ = cb_fub_heuristic->getNumLocalCutsAdded();
            num_user_branches_ = cb_node_select->getNumUserBranches();

            cout << "fub_gap_:" << fub_gap_ << ", num_local_cuts_added: " << num_local_cuts_added_ << ", num_user_branches: " << num_user_branches_ << endl;

            /* resolve without heuristic */
            cplex_.remove(cb_fub_heuristic);
            cplex_.remove(cb_node_select);
            cplex_.remove(cb_branch);
            if (num_local_cuts_added_ > 0)
                m_.add(IloRange(*env_, 0, vars_.t - cy_expr_, IloInfinity));
        } else {
            num_local_cuts_added_ = 0;
            num_user_branches_ = 0;
        }

        ticToc_ = chrono::system_clock::now() - start_t;
        elapsed_time_ = ticToc_.count();
        setTimeLimit(timelimit_ - elapsed_time_);
        
        if (!cplex_.solve()) {
            env_->error() << "Failed to optimize master." << endl;
            throw (-1);
        }

        if (cplex_.getStatus() == IloAlgorithm::Status::Unbounded)
            objVal_ = -3e+6;
        else {
            // objVal_ = cplex_.getObjValue();
            objVal_ = lazyData_.current_best_ub;
        }
        bestObjVal_ = cplex_.getBestObjValue();
        cplex_.getValues(xVals_, vars_.x);
        tVal_ = cplex_.getValue(vars_.t);
        gap_ = cplex_.getMIPRelativeGap();

        ticToc_ = chrono::system_clock::now() - start_t;
    
        delete cb_benders;
        delete cb_incumbent_update;
        delete cb_fub;
        delete cb_fub_heuristic;
        delete cb_node_select;
        delete cb_branch;

    } catch (IloException &e) {
        cerr << "CPLEX found the following exception: " << e << " in master.cpp" << endl;
        e.end();
    }
    catch (...) {
        cerr << "The following unknown exception was found in master.cpp: " << endl;
    }

    return;
}