#include "gameManager.h"
#include <string>
#include <iostream>
#include "log.h"
#include "pathFinder.h"
#include <chrono>

using namespace std;
int Goods::number = 0;
int CURRENT_FRAME = 0;
int canUnload(Berth& berth, Point2d pos) {
    int x = pos.x-berth.pos.x, y=pos.y-berth.pos.y;
    if (x<0 || x>3 || y<0 || y>3) {LOGI("越界",pos,' ',berth.pos);return 0;}
    if (berth.storageSlots[x][y]==nullptr) {LOGI("可放貨");return 1;}
    else return 0;
}

void GameManager::initializeGame()
{
    // 读取地图
    string map_data;
    int robot_id = 0;
    for (int i = 0; i < MAPROWS; ++i)
    {
        cin >> map_data;
        for (int j = 0; j < MAPCOLS; ++j)
        {
            switch (map_data[j])
            {
            case '.':
                this->gameMap.setCell(i, j, MapItemSpace::MapItem::SPACE);
                break;
            case '*':
                this->gameMap.setCell(i, j, MapItemSpace::MapItem::SEA);
                break;
            case '#':
                this->gameMap.setCell(i, j, MapItemSpace::MapItem::OBSTACLE);
                break;
            case 'A':
                // 初始化机器人
                this->gameMap.setCell(i, j, MapItemSpace::MapItem::SPACE);
                this->robots.emplace_back(robot_id, Point2d(i, j));
                robot_id++;
                this->gameMap.setCell(i, j, MapItemSpace::MapItem::SPACE);
                break;
            case 'B':
                this->gameMap.setCell(i, j, MapItemSpace::MapItem::BERTH);
                break;
            default:
                break;
            }
        }
    }
    // LOGI("Log init map info");
    // LOGI(this->gameMap.drawMap());

    // 初始化机器人
    // for (int i = 0; i < ROBOTNUMS; ++i)
    //     this->robots.emplace_back(i, Point2d(-1, -1));

    // 初始化泊位
    int id, x, y, time, velocity;
    for (int i = 0; i < BERTHNUMS; ++i)
    {
        cin >> id >> x >> y >> time >> velocity;
        this->berths.emplace_back(id, Point2d(x, y), time, velocity);
    }
    LOGI("print berth init info");
    for (const auto &berth : this->berths)
        LOGI("ID: ", berth.id, " POS: ", berth.pos, " time: ", berth.time, " velocity: ", berth.velocity);

    // 初始化船舶
    int capacity;
    cin >> capacity;
    for (int i = 0; i < SHIPNUMS; ++i)
    {
        this->ships.emplace_back(i, capacity);
    }
    // for (int i = 0; i < SHIPNUMS; ++i)
    // {
    //     LOGI("Ship ", this->ships[i].id," capacity: ", this->ships[i].capacity);
    // }

    // 初始化数据读取完成
    // 让地图实时跟踪机器人位置（需要测试是否正常跟踪）
    for (Robot &robot : this->robots)
        this->gameMap.robotPosition.push_back(robot.pos);

    // 计算地图上每个点到泊位的距离
    for (const auto &berth : this->berths)
    {
        vector<Point2d> positions;
        // 泊位大小 4x4
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                positions.push_back(berth.pos + Point2d(i, j));

        this->gameMap.computeDistancesToBerthViaBFS(berth.id, positions);
    }

    // 判断机器人是否位于死点
    for (auto &robot : this->robots)
    {
        bool is_isolated = true;
        for (const auto &berth : this->berths)
        {
            if (this->gameMap.isBerthReachable(berth.id, robot.pos))
            {
                is_isolated = false;
                break;
            }
        }
        // 孤立机器人
        if (is_isolated)
            robot.status = DEATH;
    }

    // // 初始化单行路
    // this->singleLaneManager.findSingleLanes(this->gameMap);
    
    // // 打印单行路
    // std::vector<Point2d> singleLaneList = this->singleLaneManager.getSingleLanesVector();
    // LOGI("单行路数量：",singleLaneList.size());
    // this->gameMap.drawMap(nullptr,nullptr,&singleLaneList,nullptr,nullptr);

    // LOGI("Log berth 0 BFS map.");
    // LOGI(Map::drawMap(this->gameMap.berthDistanceMap[0],12));

    string ok;
    cin >> ok;
    if (ok == "OK")
    {
        LOGI("Init complete.");
        cout << "OK" << std::endl;
    }
    else
    {
        LOGE("Init fail!");
    }
}

void GameManager::processFrameData()
{
    // LOGI("processFrameData.");
    int newItemCount;
    int goodsX, goodsY, value;
    int carrying, robotX, robotY, robotState;
    int shipState, berthId;
    // 如果读取到了 EOF，则结束
    if (cin.eof())
    {
        exit(0);
    }

    cin >> this->currentFrame >> this->currentMoney;
    CURRENT_FRAME = this->currentFrame;
    // 货物生命周期维护
    for (auto& good : goods){
        // todo 边界条件
        if(good.TTL != INT_MAX && good.TTL >= 0){
            // LOGI(good.id,",initFrame:",good.initFrame,",currentFrame:",currentFrame,",TTL:",good.TTL);
            good.TTL = std::max(1000-(currentFrame - good.initFrame),-1);
        }
    }
    // 读取新增货物
    cin >> newItemCount;
    while (newItemCount--)
    {
        cin >> goodsX >> goodsY >> value;
        this->goods.emplace_back(Point2d(goodsX, goodsY), value, currentFrame);
    }
    // 读取机器人状态
    for (int i = 0; i < ROBOTNUMS; ++i)
    {
        cin >> carrying >> robotX >> robotY >> robotState;
        this->robots[i].carryingItem = carrying;
        this->robots[i].pos.x = robotX;
        this->robots[i].pos.y = robotY;
        this->robots[i].state = robotState;

        // 暂时处理程序认为机器人放下了货物并且分配了下一个货物的id，但是判题器认为机器人还拿着上一个货物的情况
        if (this->robots[i].carryingItem == 0)
        {
            // 强行初始化机器人状态
            this->robots[i].carryingItemId = -1;
        }
        // 检查是否要 pop path
        this->robots[i].updatePath();

        // 暂时处理程序认为机器人放下了货物并且分配了下一个货物的id，但是判题器认为机器人还拿着上一个货物的情况
        if(this->robots[i].carryingItem == 0){
            // 强行初始化机器人状态
            this->robots[i].carryingItemId = -1;
        }
    }
    // 读取船舶状态
    for (int i = 0; i < SHIPNUMS; ++i)
    {
        cin >> shipState >> berthId;
        this->ships[i].state = shipState;
        this->ships[i].berthId = berthId;
    }
    // 确认已接收完本帧的所有数据
    string ok;
    cin >> ok;

    // 初始化泊位货物状态
    for(auto &berth : berths){
        berth.unreached_goods = std::vector<Goods>();
        berth.reached_goods = std::vector<Goods>();
    }

    // for (int i = 0; i < ROBOTNUMS; ++i)
    // {
    //     LOGI("Robot: ", this->robots[i].id, " position: ", this->robots[i].pos, " state: ", this->robots[i].state);
    // }
}

void GameManager::robotControl()
{
    // bool robotDebugOutput = true;
    // using std::vector;
    // vector<Action> robotsAction;
    // // 调度算法
    // for (int i =0 ; i < robots.size(); ++i) {
    //     Robot& robot = robots[i];
    //     if (robot.state == 0)
    //         robot.status = DIZZY;
        
    //     if(robot.status == DIZZY){
    //         // robotsAction.push_back(Action(ActionType::FAIL));
    //     }
    //     else if(robot.status == DEATH){
    //         // robotsAction.push_back(Action(ActionType::FAIL));
    //     }
    //     else if(robot.status == UNLOADING){
    //         LOGE("UNLOADING 状态未被处理");
    //         // robotsAction.push_back(Action(ActionType::FAIL));
    //     }
    //     else if(robot.status == IDLE){
    //         robotsAction.push_back(scheduler->scheduleRobot(robot, gameMap, goods, berths, robotDebugOutput));
    //     }
    //     else if (robot.status == MOVING_TO_GOODS){
    //         robotsAction.push_back(Action(ActionType::CONTINUE));
    //     }
    //     else if (robot.status == MOVING_TO_BERTH){
    //         // 分配泊位
    //         if (robot.targetid == -1)
    //             robotsAction.push_back(scheduler->scheduleRobot(robot, gameMap, goods, berths, robotDebugOutput));
    //         else
    //             robotsAction.push_back(Action(ActionType::CONTINUE));
    //     }
    //     else{
    //         LOGE("未被处理的状态");
    //     }
    // }

    // // 执行动作
    // for (int i =0 ; i < robots.size(); ++i)
    // {
    //     if(robotsAction[i].type == ActionType::MOVE_TO_BERTH || robotsAction[i].type == ActionType::MOVE_TO_POSITION){
    //         if(robots[i].findPath(gameMap))
    //         {

    //         }
    //         else{

    //         }
    //     }
    //     else if (robotsAction[i].type == ActionType::CONTINUE)
    //     {
    //     }
        
    // }


    
}


void GameManager::RobotControl()
{
    bool robotDebugOutput = true;
    AStarPathfinder pathfinder;
    for (int i=0;i<robots.size();i++) {
        Robot& robot = robots[i];
        if (robot.status == DEATH) continue;

        if (robot.status == DIZZY || robot.state == 0) {
            robot.status = DIZZY;
            // 还在眩晕状态
            if (robot.state == 0) continue;

            // 从眩晕状态恢复
            if (robotDebugOutput) LOGI("从眩晕状态恢复");

            if (robot.carryingItem == 0) {
                robot.status = IDLE;
                robot.path = Path();
                robot.destination = Point2d(0,0);
                robot.targetid = -1;
            }
            else {
                robot.status = MOVING_TO_BERTH;
                robot.path = Path();
                robot.targetid = -1;
            }
        }

        if (robot.status == IDLE) {
            Action action = this->scheduler->scheduleRobot(robots[i], gameMap, goods, berths, robotDebugOutput);
            if (action.type==FAIL) continue;
            std::variant<Path, PathfindingFailureReason> path = pathfinder.findPath(robots[i].pos, action.desination, gameMap);
            if (std::holds_alternative<Path>(path)) {
                if (robotDebugOutput) LOGI(i, "寻路成功");
                Path temp_path = std::get<Path>(path);
                robot.path = temp_path;
                robot.status = MOVING_TO_GOODS;
                robot.targetid = action.targetId;
                robot.destination = action.desination;
                // if(robotDebugOutput && i == 1){LOGI("分配路径：");LOGI(robots[i]);LOGI("目的地位置：",action.desination);LOGI("货物位置：",goods[robots[i].targetid].pos);LOGI("货物TTL：",goods[robots[i].targetid].TTL);}
            }
            else {
                if (robotDebugOutput) LOGI(i, "寻路失败");
                // 重置窗台
                robots[i].path = Path();
                robot.status = IDLE;
                robot.targetid = -1;
                // robot.destination = Point2d(-1,-1);
            }
        }

        if (robot.status == MOVING_TO_GOODS) {
            if (!robot.path.empty()) {
                const std::string temp = robot.moveWithPath();
                if (robotDebugOutput) LOGI(i, "向货物移动中:",temp, robot.path.size());
                commandManager.addRobotCommand(temp);
                continue; // 将移动和取货分开，防止碰撞导致的取货失败，但是将多消耗一帧
            }

            // // 看看有没有更优的货物
            // Action action = this->scheduler->scheduleRobot(robots[i], gameMap, goods, berths, robotDebugOutput);
            // if (action.type!=FAIL) {
            //     if (action.targetId != robot.targetid) {
            //         std::variant<Path, PathfindingFailureReason> path = pathfinder.findPath(robot.pos, action.desination, gameMap);
            //         if (std::holds_alternative<Path>(path)) {
            //             // if (robotDebugOutput) LOGI(i, "寻路成功");
            //             Path temp_path = std::get<Path>(path);
            //             robot.path = temp_path;
            //             robot.targetid = action.targetId;
            //             robot.destination = action.desination;
            //             // if(robotDebugOutput && i == 1){LOGI("分配路径：");LOGI(robots[i]);LOGI("目的地位置：",action.desination);LOGI("货物位置：",goods[robots[i].targetid].pos);LOGI("货物TTL：",goods[robots[i].targetid].TTL);}
            //         }
            //         else {
            //             if (robotDebugOutput) LOGI(i, "寻路失败");
            //             // 重置窗台
            //             robots[i].path = Path();
            //             robot.status = IDLE;
            //             robot.targetid = -1;
            //             // robot.destination = Point2d(-1,-1);
            //         }
            //     }
            // }

            // 如果为达到目标货物，但货物为空
            if (robot.pos != robot.destination && robot.path.empty()) {
                std::variant<Path, PathfindingFailureReason> path = pathfinder.findPath(robot.pos, robot.destination, gameMap);
                if (std::holds_alternative<Path>(path)) {
                    Path temp_path = std::get<Path>(path);
                    robot.path = temp_path;
                }
                else {
                    LOGI("已分配货物，但路径为空，且寻路失败.",robot.pos,',',robot.destination);
                    robot.status = IDLE;
                    robot.targetid = -1;
                }
            }
            // 到达货物位置
            Goods& good = goods[robot.targetid];
            if (robot.pos == good.pos) {
                // 可以取货
                if (good.TTL>0) {
                    commandManager.addRobotCommand(robot.get());
                    robots[i].carryingItem = 1;
                    robots[i].carryingItemId = robots[i].targetid;
                    robot.status = MOVING_TO_BERTH;
                    robot.targetid = -1;

                    good.TTL = INT_MAX;
                }
                // 货物过期
                else {
                    robot.status = IDLE;
                    robot.targetid = -1;
                    continue;
                }
            }
            continue;
        }

        if (robot.status == MOVING_TO_BERTH) {
            // 分配泊位
            if (robot.targetid == -1) {
                Action action = this->scheduler->scheduleRobot(robot, gameMap, goods, berths, robotDebugOutput);
                std::variant<Path, PathfindingFailureReason> path = pathfinder.findPath(robot.pos, action.desination, gameMap);
                if (std::holds_alternative<Path>(path)) {
                    Path temp_path = std::get<Path>(path);
                    robot.path = temp_path;
                    robot.status = MOVING_TO_BERTH;
                    robot.targetid = action.targetId;
                    robot.destination = action.desination;
                }
                else {
                    robot.targetid = -1;
                }
                continue;
            }

            // 如果未到达目标泊位，但是路径为空
            if (robot.pos != robot.destination && robot.path.empty()) {
                std::variant<Path, PathfindingFailureReason> path = pathfinder.findPath(robot.pos, robot.destination, gameMap);
                if (std::holds_alternative<Path>(path)) {
                    Path temp_path = std::get<Path>(path);
                    robot.path = temp_path;
                }
                else {
                    LOGI("已分配泊位，但路径为空，且寻路失败.",robot.pos,',',robot.destination);
                    robot.targetid = -1;
                }
            }
            
            const std::string temp = robot.moveWithPath();
            if (robotDebugOutput) LOGI(i, "向泊位移动中:",temp);
            commandManager.addRobotCommand(temp);

            // 放货
            Berth &berth = berths[robot.targetid];
            LOGI(robot);
            // berth.info(); //这个函数有bug，
            if (robot.pos == robot.destination) {
                // 货物可放货 todo
                LOGI("放貨",robot,"dest",robot.destination);
                if (canUnload(berth, robot.pos)) {
                    LOGI("輸出");
                    commandManager.addRobotCommand(robots[i].pull());
                    // 货物 泊位 状态更新 todo
                    int x = robot.pos.x-berth.pos.x, y=robot.pos.y-berth.pos.y;
                    berth.storageSlots[x][y] = &goods[robot.carryingItemId];
                    berth.reached_goods.push_back(goods[robot.carryingItemId]);


                    goods[robot.carryingItemId].status = 3;
                    // goods[robot.carryingItemId].TTL = ;

                    robot.status = IDLE;
                    robot.carryingItem = 0;
                    robot.carryingItemId = -1;
                    robot.targetid = -1;
                }
                // 不可放货，重新分配放货位置
                else {
                    LOGI(robot);
                    // berth.info();
                    robot.targetid = -1;
                }
            }
        }

    }
}



void GameManager::update()
{   
    LOGI("进入update函数-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");
    auto start = std::chrono::steady_clock::now();
    
    bool robotDebugOutput = true;
    bool shipDebugOutput = false;
    RobotControl();

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    if(robotDebugOutput) LOGI("调度机器人时长:",duration.count(),"ms");

    if(shipDebugOutput){LOGI("船只开始调度");};
    auto ship_start = std::chrono::high_resolution_clock::now();
    std::vector<std::pair<int, Action>> ShipActions = this->scheduler->scheduleShips(ships, berths, goods, robots, shipDebugOutput);
    auto ship_end = std::chrono::high_resolution_clock::now();
    if(shipDebugOutput) LOGI("调度船只时长:",std::chrono::duration_cast<std::chrono::milliseconds>(ship_end - ship_start).count(),"ms");

    // CommandManager.shipCommands
    for (int i = 0; i < ShipActions.size(); i++)
    {
        int ship_id = ShipActions[i].first;
        Action ship_action = ShipActions[i].second;
        // 去虚拟点
        if (ship_action.type == DEPART_BERTH)
        {
            // LOGI(ship_id,"前往虚拟点");
            commandManager.addShipCommand(ships[ship_id].go());
        }
        // 去泊位
        if (ship_action.type == MOVE_TO_BERTH)
        {
            // LOGI(ship_id,"分配去泊位",ship_action.targetId);
            commandManager.addShipCommand(ships[ship_id].moveToBerth(ship_action.targetId));
        }
    }
}

void GameManager::outputCommands()
{

    commandManager.outputCommands();
    commandManager.clearCommands();
}
