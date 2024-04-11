#pragma once
#include "map.h"

struct Params
{
    // 注意命名规范，防止不同的子类超参互相冲突
    // robot 参数
    // 例如，float greedyRobotScheduleTTLProfitWeight
    // 泊位聚类超参
    int CLUSTERNUMS = 4;                    // 泊位的聚类类别数目

    // 机器人调度超参
    float TTL_ProfitWeight = 1.5;
    int TTL_Bound = 500;
    bool PartitionScheduling = true;        // 是否分区调度
    bool DynamicPartitionScheduling = true; // 是否动态分区调度
    std::vector<int> ASSIGNBOUND;           // 手动设置各个类分配的机器人数目，总数目应等于机器人数目
    float robotReleaseBound = 0.5;          //低于平均泊位价值的比值时，释放机器人去其他泊位
    int DynamicSchedulingInterval = 200;    // 动态调度间隔
    
    // 购买策略超参
    int maxRobotNum = 12;                   // 最多购买机器人数目
    int maxShipNum = 3;                     // 最多购买船只数目
    std::vector<std::vector<int>> robotPurchaseAssign = {{8, 100}, {1, 4}, {1, 4}};
    // std::vector<std::vector<int>> shipPurchaseAssign = {{4, 4, 4, 5, 6, 7, 8, 9, 10}, {1, 2, 3, 4, 5, 6}, {1, 2, 3, 4, 5, 6}};
    std::vector<std::vector<int>> shipPurchaseAssign = {{1, 4, 10}, {0, 0, 0}, {0, 0, 0}};
    int timeToBuyShip = 50;                 // 开始购买第二艘船的时间
    int startNum = 1;                       // 最初的数目（机器人、轮船）
    float landDistanceWeight = 10.0;        // 对泊位价值评估时的陆地访问距离权重
    float deliveryDistanceWeight = 10.0;    // 对泊位价值评估时的交货点访问距离权重
    bool CentralizedTransportation = true;  // 游戏开局是否集中调度
    bool robotFirst = true;                 // 先买机器人还是先买船，true为机器人、false为船

    //  泊位超参
    float ABLE_DEPART_SCALE = 0.15;         //可以去虚拟点的剩余容量比例
    int MAX_SHIP_NUM = 1;                   // 一个泊位最多几艘船
    int TIME_TO_WAIT = 100;                 //等待有货的时间段
    int CAPACITY_GAP = 10;                  // 泊位溢出货物量和船的容量差
    int SHIP_WAIT_TIME_LIMIT = 5;           //船在泊位上等待货物的时间

    // ship 参数

    Params(MapFlag mapFalg) {
        if (mapFalg == MapFlag::NORMAL) {

        }
        else if (mapFalg == MapFlag::ERROR) {

        }
    }
};
