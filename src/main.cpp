#include <iostream>
#include <robot_dart/control/robot_control.hpp>
#include <robot_dart/robot_dart_simu.hpp>

#include <dart/collision/bullet/BulletCollisionDetector.hpp>
#include <dart/constraint/ConstraintSolver.hpp>

#ifdef GRAPHIC
#include <robot_dart/graphics/graphics.hpp>
#endif

#include <dart/collision/bullet/BulletCollisionDetector.hpp>
#include <dart/constraint/ConstraintSolver.hpp>

#include <icub/model/iCub.hpp>

#include <icub/solver/qp_solver.hpp>

#include <chrono>

// class QPControl : public robot_dart::control::RobotControl {
// public:
//     QPControl() : robot_dart::control::RobotControl() {}

//     void configure() override
//     {
//         _active = true;
//         auto robot = _robot.lock();
//         _solver = std::make_shared<icub::solver::QPSolver>(robot);
//         // _solver->add_task(icub::task::create_task<icub::task::AccelerationTask>(robot->skeleton(), "iiwa_link_ee", Eigen::VectorXd::Zero(6)), 10000.);
//         // _solver->add_task(icub::task::create_task<icub::task::DirectTrackingTask>(robot->skeleton(), Eigen::VectorXd::Zero(robot->skeleton()->getNumDofs() * 2)), 0.01);
//         // _solver->add_constraint(icub::constraint::create_constraint<icub::constraint::DynamicsConstraint>(robot->skeleton(), false));
//     }

//     Eigen::VectorXd calculate(double) override
//     {
//         auto robot = _robot.lock();
//         _solver->clear_all();
//         Eigen::VectorXd target = Eigen::VectorXd::Zero(robot->skeleton()->getNumDofs() * 2);
//         target.tail(robot->skeleton()->getNumDofs()) = robot->skeleton()->getCoriolisAndGravityForces();
//         _solver->add_task(icub::task::create_task<icub::task::AccelerationTask>(robot->skeleton(), "iiwa_link_ee", Eigen::VectorXd::Zero(6)), 10000.);
//         _solver->add_task(icub::task::create_task<icub::task::DirectTrackingTask>(robot->skeleton(), target), 0.01);
//         _solver->add_constraint(icub::constraint::create_constraint<icub::constraint::DynamicsConstraint>(robot->skeleton(), false));
//         _solver->solve();

//         Eigen::VectorXd commands = _solver->solution().tail(robot->skeleton()->getNumDofs());
//         // std::cout << commands.transpose() << std::endl;
//         // // std::cout << robot->skeleton()->getAccelerations().transpose() << std::endl;
//         // std::cout << robot->skeleton()->getCoriolisAndGravityForces().transpose() << std::endl;
//         // std::cout << std::endl;
//         return commands;
//         // return robot->skeleton()->getCoriolisAndGravityForces();
//     }

//     std::shared_ptr<robot_dart::control::RobotControl> clone() const
//     {
//         return std::make_shared<QPControl>(*this);
//     }

// protected:
//     std::shared_ptr<icub::solver::QPSolver> _solver;
// };

class QPControl : public robot_dart::control::RobotControl {
public:
    QPControl() : robot_dart::control::RobotControl() {}

    void configure() override
    {
        _active = true;
        auto robot = _robot.lock();
        _solver = std::make_shared<icub::solver::QPSolver>(robot);
        _prev_tau = Eigen::VectorXd::Zero(robot->skeleton()->getNumDofs());
    }

    Eigen::VectorXd calculate(double t) override
    {
        auto robot = _robot.lock();

        // Eigen::VectorXd actual_com = robot->skeleton()->getPositions().head(6);
        // Eigen::VectorXd desired_com = Eigen::VectorXd::Zero(6);
        // desired_com(5) = 0.62426;

        // double Kp = 1.;
        // double Kd = 0.1;
        // Eigen::VectorXd desired_acc = Kp * (desired_com - actual_com) - Kd * robot->skeleton()->getVelocities().head(6);

        // Eigen::Isometry3d tr_rhand = robot->skeleton()->getBodyNode("r_hand")->getWorldTransform();
        // Eigen::VectorXd actual_rhand(6);
        // actual_rhand.tail(3) = tr_rhand.translation();
        // actual_rhand.head(3) = dart::math::quatToExp(Eigen::Quaterniond(tr_rhand.linear()));
        // Eigen::VectorXd desired_rhand(6);
        // desired_rhand << 1.57833e-12, -0.776671, 4.76067e-12, -0.142085, 0.0898981, 0.557613;
        // Eigen::VectorXd desired_acc_rhand = Kp * (desired_rhand - actual_rhand) - Kd * robot->skeleton()->getBodyNode("r_hand")->getSpatialVelocity();

        _solver->clear_all();
        Eigen::VectorXd target = Eigen::VectorXd::Zero(robot->skeleton()->getNumDofs() * 2 + 2 * 6);
        target.segment(robot->skeleton()->getNumDofs(), robot->skeleton()->getNumDofs()).tail(_control_dof) = robot->skeleton()->getCoriolisAndGravityForces().tail(_control_dof);
        double task_weight = 10000.;
        double gen_weight = 0.1;

        // _solver->add_task(icub::task::create_task<icub::task::COMAccelerationTask>(robot->skeleton(), desired_acc), task_weight);
        // // _solver->add_task(icub::task::create_task<icub::task::AccelerationTask>(robot->skeleton(), "head", Eigen::VectorXd::Zero(6)), task_weight);
        // _solver->add_task(icub::task::create_task<icub::task::AccelerationTask>(robot->skeleton(), "r_hand", desired_acc_rhand), task_weight);
        // // _solver->add_task(icub::task::create_task<icub::task::AccelerationTask>(robot->skeleton(), "l_hand", Eigen::VectorXd::Zero(6)), task_weight);
        _solver->add_task(icub::task::create_task<icub::task::COMAccelerationTask>(robot->skeleton(), Eigen::VectorXd::Zero(6)), task_weight);
        _solver->add_task(icub::task::create_task<icub::task::AccelerationTask>(robot->skeleton(), "head", Eigen::VectorXd::Zero(6)), task_weight);
        _solver->add_task(icub::task::create_task<icub::task::AccelerationTask>(robot->skeleton(), "r_hand", Eigen::VectorXd::Zero(6)), task_weight);
        _solver->add_task(icub::task::create_task<icub::task::AccelerationTask>(robot->skeleton(), "l_hand", Eigen::VectorXd::Zero(6)), task_weight);
        // regularization
        _solver->add_task(icub::task::create_task<icub::task::DirectTrackingTask>(robot->skeleton(), target), gen_weight);
        _solver->add_constraint(icub::constraint::create_constraint<icub::constraint::DynamicsConstraint>(robot->skeleton()));
        // _solver->add_task(icub::task::create_task<icub::task::TauDiffTask>(robot->skeleton(), _prev_tau), gen_weight);
        Eigen::VectorXd up(3), t1(3), t2(3);
        up << 0., 0., 1.;
        t1 << 1., 0., 0.;
        t2 << 0., 1., 0.;
        icub::constraint::Contact c;
        c.mu = 1.;
        c.normal = up;
        c.t1 = t1;
        c.t2 = t2;
        c.min_force = 0.;
        c.max_force = 500.;
        c.min = Eigen::VectorXd::Zero(6);
        c.max = Eigen::VectorXd::Zero(6);
        c.max.tail(3) << c.max_force, c.max_force, c.max_force;

        _solver->add_contact(task_weight, "r_sole", c);
        _solver->add_contact(task_weight, "l_sole", c);

        _solver->solve();

        // std::cout << _solver->solution().tail(2 * 6).transpose() << std::endl;
        Eigen::VectorXd commands = _solver->solution().segment(robot->skeleton()->getNumDofs(), robot->skeleton()->getNumDofs()).tail(_control_dof);
        _prev_tau = _solver->solution().segment(robot->skeleton()->getNumDofs(), robot->skeleton()->getNumDofs());
        // std::cout << t << ":\n";
        // std::cout << "    " << commands.transpose() << std::endl;
        // // std::cout << robot->skeleton()->getAccelerations().transpose() << std::endl;
        // std::cout << "    " << robot->skeleton()->getCoriolisAndGravityForces().tail(_control_dof).transpose() << std::endl;
        // std::cout << std::endl;
        // commands = robot->skeleton()->getCoriolisAndGravityForces().tail(_control_dof);
        return commands;
        // return robot->skeleton()->getCoriolisAndGravityForces();
    }

    std::shared_ptr<robot_dart::control::RobotControl> clone() const
    {
        return std::make_shared<QPControl>(*this);
    }

protected:
    std::shared_ptr<icub::solver::QPSolver> _solver;
    Eigen::VectorXd _prev_tau;
};

int main()
{
    // std::vector<std::pair<std::string, std::string>> packages = {{"iiwa_description", std::string(RESPATH) + "/iiwa/"}};
    // auto arm = std::make_shared<robot_dart::Robot>(std::string(RESPATH) + "/iiwa/iiwa14.urdf", packages);
    // // auto arm = std::make_shared<robot_dart::Robot>(std::string(RESPATH) + "/arm.urdf");
    // arm->fix_to_world();
    // arm->skeleton()->setPosition(1, M_PI / 3.);

    // icub::solver::QPSolver qp(arm);
    // // qp.add_task(icub::task::create_task<icub::task::AccelerationTask>(arm->skeleton(), "arm_link_5", Eigen::VectorXd::Zero(6)), 20.);
    // qp.add_task(icub::task::create_task<icub::task::AccelerationTask>(arm->skeleton(), "iiwa_link_ee", Eigen::VectorXd::Zero(6)), 20.);
    // qp.add_task(icub::task::create_task<icub::task::DirectTrackingTask>(arm->skeleton(), Eigen::VectorXd::Zero(arm->skeleton()->getNumDofs() * 2)));
    // qp.add_constraint(icub::constraint::create_constraint<icub::constraint::DynamicsConstraint>(arm->skeleton(), false));
    // qp.solve();
    // // // std::srand(std::time(NULL));
    icub::model::iCub icub("MyiCub");

    // arm->add_controller(std::make_shared<QPControl>());
    auto icub_robot = icub.robot();
    icub_robot->set_position_enforced(true);
    icub_robot->skeleton()->setPosition(5, 0.625);
    // icub_robot->skeleton()->disableSelfCollision();
    icub_robot->add_controller(std::make_shared<QPControl>());

    // std::cout << icub_robot->skeleton()->getPositions().head(6).transpose() << std::endl;
    // Eigen::Isometry3d tr_rhand = icub_robot->skeleton()->getBodyNode("r_hand")->getWorldTransform();
    // Eigen::VectorXd actual_rhand(6);
    // actual_rhand.tail(3) = tr_rhand.translation();
    // actual_rhand.head(3) = dart::math::quatToExp(Eigen::Quaterniond(tr_rhand.linear()));
    // std::cout << actual_rhand.transpose() << std::endl;

    Eigen::VectorXd lb, ub;
    lb = icub_robot->skeleton()->getForceLowerLimits();
    ub = icub_robot->skeleton()->getForceUpperLimits();

    lb = lb.unaryExpr([](double x) {if(x<-84.) return -84.; return x; });
    ub = ub.unaryExpr([](double x) {if(x>84.) return 84.; return x; });
    lb.head(6) = Eigen::VectorXd::Zero(6);
    ub.head(6) = Eigen::VectorXd::Zero(6);
    icub_robot->skeleton()->setForceLowerLimits(lb);
    icub_robot->skeleton()->setForceUpperLimits(ub);

    lb = icub_robot->skeleton()->getVelocityLowerLimits();
    ub = icub_robot->skeleton()->getVelocityLowerLimits();

    lb = lb.unaryExpr([](double x) {if(x<-100.) return -100.; return x; });
    ub = ub.unaryExpr([](double x) {if(x>100.) return 100.; return x; });
    lb.head(6) = Eigen::VectorXd::Zero(6);
    ub.head(6) = Eigen::VectorXd::Zero(6);
    icub_robot->skeleton()->setVelocityLowerLimits(lb);
    icub_robot->skeleton()->setVelocityLowerLimits(ub);

    robot_dart::RobotDARTSimu simu(0.005);
    simu.world()->getConstraintSolver()->setCollisionDetector(dart::collision::BulletCollisionDetector::create());
#ifdef GRAPHIC
    simu.set_graphics(std::make_shared<robot_dart::graphics::Graphics>(simu.world()));
#endif
    simu.add_robot(icub_robot);
    simu.add_floor();
    simu.run(10.);

    // std::cout << icub_robot->skeleton()->getPositions().head(6).transpose() << std::endl;

    // icub::solver::QPSolver qp(icub.robot());
    // double task_weight = 10000.;
    // double gen_weight = 0.1;
    // auto start = std::chrono::steady_clock::now();
    // qp.add_task(icub::task::create_task<icub::task::COMAccelerationTask>(icub.skeleton(), Eigen::VectorXd::Zero(6)), task_weight);
    // qp.add_task(icub::task::create_task<icub::task::AccelerationTask>(icub.skeleton(), "r_hand", Eigen::VectorXd::Zero(6)), task_weight);
    // qp.add_task(icub::task::create_task<icub::task::AccelerationTask>(icub.skeleton(), "l_hand", Eigen::VectorXd::Zero(6)), task_weight);
    // // regularization
    // qp.add_task(icub::task::create_task<icub::task::DirectTrackingTask>(icub.skeleton(), Eigen::VectorXd::Zero(icub.skeleton()->getNumDofs() * 2 + 2 * 6)), gen_weight);
    // qp.add_constraint(icub::constraint::create_constraint<icub::constraint::DynamicsConstraint>(icub.skeleton()));
    // Eigen::VectorXd up(3), t1(3), t2(3);
    // up << 0., 0., 1.;
    // t1 << 1., 0., 0.;
    // t2 << 0., 1., 0.;
    // icub::constraint::Contact c;
    // c.mu = 1.;
    // c.normal = up;
    // c.t1 = t1;
    // c.t2 = t2;
    // c.min_force = 0.;
    // c.max_force = 500.;
    // c.min = Eigen::VectorXd::Zero(6);
    // c.max = Eigen::VectorXd::Zero(6);
    // c.max.tail(3) << c.max_force, c.max_force, c.max_force;

    // qp.add_contact(task_weight, "r_sole", c);
    // qp.add_contact(task_weight, "l_sole", c);
    // // qp.add_task(icub::task::create_task<icub::task::AccelerationTask>(icub.skeleton(), "r_sole", Eigen::VectorXd::Zero(6)));
    // // qp.add_task(icub::task::create_task<icub::task::AccelerationTask>(icub.skeleton(), "l_sole", Eigen::VectorXd::Zero(6)));
    // // qp.solve(Eigen::VectorXd::Zero(5 * 6));
    // qp.solve();
    // auto end = std::chrono::steady_clock::now();
    // auto diff = end - start;
    // std::cout << std::chrono::duration<double, std::milli>(diff).count() << " ms" << std::endl;

    // qp.clear_all();
    // start = std::chrono::steady_clock::now();
    // qp.add_task(icub::task::create_task<icub::task::COMAccelerationTask>(icub.skeleton(), Eigen::VectorXd::Zero(6)), task_weight);
    // qp.add_task(icub::task::create_task<icub::task::AccelerationTask>(icub.skeleton(), "r_hand", Eigen::VectorXd::Ones(6).array() * 0.1), task_weight);
    // qp.add_task(icub::task::create_task<icub::task::AccelerationTask>(icub.skeleton(), "l_hand", Eigen::VectorXd::Zero(6)), task_weight);
    // // regularization
    // qp.add_task(icub::task::create_task<icub::task::DirectTrackingTask>(icub.skeleton(), Eigen::VectorXd::Zero(icub.skeleton()->getNumDofs() * 2 + 2 * 6)), gen_weight);
    // qp.add_constraint(icub::constraint::create_constraint<icub::constraint::DynamicsConstraint>(icub.skeleton()));

    // qp.add_contact(task_weight, "r_sole", c);
    // qp.add_contact(task_weight, "l_sole", c);
    // qp.solve();
    // end = std::chrono::steady_clock::now();
    // diff = end - start;
    // std::cout << std::chrono::duration<double, std::milli>(diff).count() << " ms" << std::endl;

    //     auto iCub_robot = icub.robot();
    //     iCub_robot->free_from_world();
    //     iCub_robot->set_position_enforced(true);
    //     iCub_robot->skeleton()->setPosition(5, 0.8);

    //     iCub_robot->set_actuator_types(dart::dynamics::Joint::SERVO);
    //     for (int i = 0; i < 6; i++)
    //         iCub_robot->skeleton()->getDof(i)->getJoint()->setActuatorType(dart::dynamics::Joint::FORCE);

    //     robot_dart::RobotDARTSimu simu(0.001);

    //     simu.world()->getConstraintSolver()->setCollisionDetector(dart::collision::BulletCollisionDetector::create());
    // #ifdef GRAPHIC
    //     simu.set_graphics(std::make_shared<robot_dart::graphics::Graphics>(simu.world()));
    // #endif
    //     simu.add_robot(iCub_robot);
    //     simu.add_floor(10., 0.2);

    //     simu.run(5);

    // /* Setup data of first QP. */
    // qpOASES::real_t H[2 * 2] = {1.0, 0.0, 0.0, 0.5};
    // qpOASES::real_t A[1 * 2] = {1.0, 1.0};
    // qpOASES::real_t g[2] = {1.5, 1.0};
    // qpOASES::real_t lb[2] = {0.5, -2.0};
    // qpOASES::real_t ub[2] = {5.0, 2.0};
    // qpOASES::real_t lbA[1] = {-1.0};
    // qpOASES::real_t ubA[1] = {2.0};
    // /* Setup data of second QP. */
    // qpOASES::real_t g_new[2] = {1.0, 1.5};
    // qpOASES::real_t lb_new[2] = {0.0, -1.0};
    // qpOASES::real_t ub_new[2] = {5.0, -0.5};
    // qpOASES::real_t lbA_new[1] = {-2.0};
    // qpOASES::real_t ubA_new[1] = {1.0};
    // /* Setting up QProblem object. */
    // std::cout << "Starting example" << std::endl;
    // qpOASES::QProblem example(2, 1);
    // auto options = example.getOptions();
    // options.printLevel = qpOASES::PL_LOW;
    // example.setOptions(options);
    // /* Solve first QP. */
    // int nWSR = 10;
    // example.init(H, g, A, lb, ub, lbA, ubA, nWSR);
    // std::cout << "Finished example" << std::endl;

    return 0;
}
