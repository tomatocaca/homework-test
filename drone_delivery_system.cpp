#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <ctime>
using namespace std;

// =====================================================================
// 工具函数：清屏（仿照原始代码，Windows用cls，其他用clear）
// =====================================================================
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// =====================================================================
// 工具函数：等待用户按 'q' 返回（仿照原始代码的waitForReturn）
// =====================================================================
void waitForReturn() {
    cout << "\n按 'q' 键返回: ";
    char ch;
    cin >> ch;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// =====================================================================
// 枚举定义
// =====================================================================

// 货物类型枚举：普通货物、生鲜、药品、贵重物品
enum CargoType {
    Normal,    // 普通货物 —— 轻型及以上无人机可运送
    Fresh,     // 生鲜     —— 中型及以上无人机可运送（需温控）
    Medicine,  // 药品     —— 中型及以上无人机可运送（需温控）
    Valuable   // 贵重物品 —— 仅重型无人机可运送（需保险锁）
};

// 订单状态枚举：从用户下单到收货的完整生命周期
enum OrderState {
    Pending,      // 待处理 —— 用户刚下单，配送员尚未分配
    Assigned,     // 已分配 —— 配送员已将订单分配给某架无人机
    Delivering,   // 配送中 —— 无人机正在执行配送
    Delivered,    // 已送达 —— 无人机已到达目的地，等待用户确认
    Received,     // 已收货 —— 用户已确认收货（终态）
    Cancelled     // 已取消 —— 用户主动取消订单（终态）
};

// 无人机状态枚举
enum DroneState {
    Idle,          // 空闲 —— 可接受新配送任务
    DRDelivering   // 配送中 —— 正在执行配送任务
};

// 无人机等级枚举：决定载重上限和可运送货物类型
enum DroneLevel {
    LightDroneLevel,   // 轻型 —— ≤5kg，仅普通货物
    MediumDroneLevel,  // 中型 —— ≤20kg，普通+生鲜+药品，温控
    HeavyDroneLevel    // 重型 —— ≤50kg，全品类，温控+保险锁
};

// =====================================================================
// 全局常量：各类无人机的性能参数
// =====================================================================
const double LIGHT_MAX_WEIGHT = 5.0;    // 轻型无人机最大载重(kg)
const double MEDIUM_MAX_WEIGHT = 20.0;  // 中型无人机最大载重(kg)
const double HEAVY_MAX_WEIGHT = 50.0;   // 重型无人机最大载重(kg)
const int MAX_DAILY_DELIVERIES = 5;     // 每架无人机每日最多配送次数

// =====================================================================
// 配送地图：4个站点，相邻站点之间配送时长10分钟
// 线路：A(天马公寓) — B(德智公寓) — C(综合楼) — D(东方红广场)
// =====================================================================
// 站点名称数组，索引0=A, 1=B, 2=C, 3=D
const string STATION_NAMES[4] = {"天马公寓", "德智公寓", "综合楼", "东方红广场"};
const string STATION_CODES[4] = {"A", "B", "C", "D"};       // 站点代号
const int SEGMENT_TIME = 10;    // 相邻站点间配送时长（分钟）

// =====================================================================
// 根据起点和终点代号计算配送时长（分钟）
// 线路为 A—B—C—D 线性，距离 = |index差| × SEGMENT_TIME
// =====================================================================
int calcDeliveryTime(const string& from, const string& to) {
    int fi = -1, ti = -1;
    for (int i = 0; i < 4; i++) {
        if (STATION_CODES[i] == from) fi = i;
        if (STATION_CODES[i] == to)   ti = i;
    }
    if (fi == -1 || ti == -1) return 0;   // 无效站点
    int dist = fi > ti ? fi - ti : ti - fi;  // |索引差|
    return dist * SEGMENT_TIME;               // 距离×10分钟
}

// =====================================================================
// 根据站点代号获取站点中文名
// =====================================================================
string getStationName(const string& code) {
    for (int i = 0; i < 4; i++) {
        if (STATION_CODES[i] == code) return STATION_NAMES[i];
    }
    return "未知";
}

// =====================================================================
// 安全的整数输入函数：防止输入非数字导致 cin 死循环刷屏
// =====================================================================
int readInt() {
    int value;
    while (!(cin >> value)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "输入无效，请输入数字: ";
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    return value;
}

// =====================================================================
// 安全的浮点数输入函数
// =====================================================================
double readDouble() {
    double value;
    while (!(cin >> value)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "输入无效，请输入数字: ";
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    return value;
}

// =====================================================================
// 全局变量
// =====================================================================
int orderCounter = 1;  // 订单序号计数器，用于生成唯一的订单编号

// =====================================================================
// 辅助函数：将货物类型枚举转为中文字符串
// =====================================================================
string cargoTypeToString(CargoType ct) {
    switch (ct) {
        case Normal:   return "普通货物";
        case Fresh:    return "生鲜";
        case Medicine: return "药品";
        case Valuable: return "贵重物品";
        default:       return "未知";
    }
}

// =====================================================================
// 辅助函数：将订单状态枚举转为中文字符串
// =====================================================================
string orderStateToString(OrderState os) {
    switch (os) {
        case Pending:    return "待处理";
        case Assigned:   return "已分配";
        case Delivering: return "配送中";
        case Delivered:  return "已送达";
        case Received:   return "已收货";
        case Cancelled:  return "已取消";
        default:         return "未知";
    }
}

// =====================================================================
// 辅助函数：将无人机状态枚举转为中文字符串
// =====================================================================
string droneStateToString(DroneState ds) {
    switch (ds) {
        case Idle:         return "空闲";
        case DRDelivering: return "配送中";
        default:           return "未知";
    }
}

// =====================================================================
// 辅助函数：将无人机等级枚举转为中文字符串
// =====================================================================
string droneLevelToString(DroneLevel dl) {
    switch (dl) {
        case LightDroneLevel:  return "轻型";
        case MediumDroneLevel: return "中型";
        case HeavyDroneLevel:  return "重型";
        default:               return "未知";
    }
}

// =====================================================================
// 辅助函数：生成唯一订单编号（格式：DD + 年月日 + 3位序号）
// =====================================================================
string generateOrderID() {
    time_t now = time(0);              // 获取当前时间戳
    tm* ltm = localtime(&now);         // 转为本地时间结构体
    stringstream ss;
    ss << "DD"
       << setw(4) << setfill('0') << (1900 + ltm->tm_year)  // 年份（4位）
       << setw(2) << setfill('0') << (1 + ltm->tm_mon)      // 月份（2位，tm_mon从0开始）
       << setw(2) << setfill('0') << ltm->tm_mday            // 日（2位）
       << setw(3) << setfill('0') << orderCounter;           // 当日序号（3位）
    orderCounter++;  // 序号递增
    return ss.str();
}

// =====================================================================
// CargoType转用户输入数字的辅助函数：让用户选择货物类型
// =====================================================================
CargoType inputCargoType() {
    int choice;
    cout << "货物类型：" << endl;
    cout << "  1. 普通货物" << endl;
    cout << "  2. 生鲜" << endl;
    cout << "  3. 药品" << endl;
    cout << "  4. 贵重物品" << endl;
    cout << "请选择 (1-4): ";
    choice = readInt();
    switch (choice) {
        case 1: return Normal;
        case 2: return Fresh;
        case 3: return Medicine;
        case 4: return Valuable;
        default:
            cout << "输入无效，默认为普通货物" << endl;
            return Normal;
    }
}

// =====================================================================
// Order 类 —— 订单：记录每一次配送请求的完整信息
// =====================================================================
class Order {
private:
    string orderID;          // 订单编号（系统自动生成，如 DD20260607001）
    CargoType cargoType;     // 货物类型（普通/生鲜/药品/贵重）
    string recipientName;    // 收件人姓名
    string recipientPhone;   // 收件人电话
    string recipientAddress; // 收件地址
    double weight;           // 货物重量(kg)
    string description;      // 货物描述
    OrderState orderState;   // 订单当前状态
    string ownerName;        // 下单用户名（user001/user002/user003）
    time_t scheduledTime;    // 预约配送时间（Unix时间戳，0表示即日送达）
    string timeSlot;         // 预约时段描述（"上午8-12" / "下午12-17" / "晚上17-21"，空串表示即日）
    string fromStation;      // 配送起点站点代号（A/B/C/D）
    string toStation;        // 配送终点站点代号（A/B/C/D）
    int deliveryMinutes;     // 预计配送时长（分钟）

public:
    // ---------- 默认构造函数：创建一个空订单 ----------
    Order() {
        orderID = "#";
        cargoType = Normal;
        recipientName = "#";
        recipientPhone = "#";
        recipientAddress = "#";
        weight = 0;
        description = "#";
        orderState = Pending;
        ownerName = "#";
        scheduledTime = 0;   // 0 表示即日（不预约）
        timeSlot = "";
        fromStation = "A";   // 默认起点天马公寓
        toStation = "A";     // 默认终点天马公寓
        deliveryMinutes = 0;
    }

    // ---------- 带参构造函数：用所有信息初始化订单 ----------
    Order(string oid, CargoType ct, string rname, string rphone,
          string raddr, double w, string desc, OrderState os, string owner,
          time_t st = 0, string ts = "",
          string fs = "A", string ds = "A", int dm = 0)
        : orderID(oid), cargoType(ct), recipientName(rname),
          recipientPhone(rphone), recipientAddress(raddr),
          weight(w), description(desc), orderState(os), ownerName(owner),
          scheduledTime(st), timeSlot(ts),
          fromStation(fs), toStation(ds), deliveryMinutes(dm) {}

    // ---------- 复制构造函数 ----------
    Order(const Order& o)
        : orderID(o.orderID), cargoType(o.cargoType),
          recipientName(o.recipientName), recipientPhone(o.recipientPhone),
          recipientAddress(o.recipientAddress), weight(o.weight),
          description(o.description), orderState(o.orderState),
          ownerName(o.ownerName), scheduledTime(o.scheduledTime),
          timeSlot(o.timeSlot), fromStation(o.fromStation),
          toStation(o.toStation), deliveryMinutes(o.deliveryMinutes) {}

    // ===== Getter 接口函数 =====
    string getOrderID() const          { return orderID; }
    CargoType getCargoType() const     { return cargoType; }
    string getRecipientName() const    { return recipientName; }
    string getRecipientPhone() const   { return recipientPhone; }
    string getRecipientAddress() const { return recipientAddress; }
    double getWeight() const           { return weight; }
    string getDescription() const      { return description; }
    OrderState getOrderState() const   { return orderState; }
    string getOwnerName() const        { return ownerName; }
    time_t getScheduledTime() const    { return scheduledTime; }
    string getTimeSlot() const         { return timeSlot; }
    string getFromStation() const      { return fromStation; }
    string getToStation() const        { return toStation; }
    int getDeliveryMinutes() const     { return deliveryMinutes; }

    // ===== Setter 函数 =====
    void setOrderState(OrderState os) { orderState = os; }

    // ---------- 显示订单完整信息 ----------
    void Show() {
        cout << "订单编号：" << orderID << endl;
        cout << "货物类型：" << cargoTypeToString(cargoType) << endl;
        cout << "货物描述：" << description << endl;
        cout << "货物重量：" << weight << " kg" << endl;
        if (scheduledTime > 0) {
            // 预约订单：显示预约日期和时段
            tm* stm = localtime(&scheduledTime);   // 将时间戳转为日期结构
            cout << "预约配送日期："
                 << (1900 + stm->tm_year) << "-"
                 << setw(2) << setfill('0') << (1 + stm->tm_mon) << "-"
                 << setw(2) << setfill('0') << stm->tm_mday
                 << "  时段：" << timeSlot << endl;
        } else {
            cout << "配送方式：即日配送" << endl;
        }
        cout << "配送路线：" << fromStation << "(" << getStationName(fromStation)
             << ") → " << toStation << "(" << getStationName(toStation)
             << ")，预计 " << deliveryMinutes << " 分钟" << endl;
        cout << "收件人：" << recipientName << "  电话：" << recipientPhone << endl;
        cout << "详细地址：" << recipientAddress << endl;
        cout << "下单用户：" << ownerName << endl;
        cout << "订单状态：" << orderStateToString(orderState) << endl;
    }
};

// =====================================================================
// 全局变量：所有订单的集合（仿照原始代码中的 baoguolist）
// 放在 Order 类定义之后、Drone 类定义之前
// =====================================================================
vector<Order> allOrders;

// =====================================================================
// Drone 基类 —— 无人机：封装所有无人机的共同属性和行为
// =====================================================================
class Drone {
protected:
    string droneID;          // 无人机编号（L/M/H + 3位数字，如 L001）
    DroneState droneState;   // 无人机当前状态
    DroneLevel level;        // 无人机等级（轻型/中型/重型）
    double maxWeight;        // 最大载重(kg)
    int deliveryCount;       // 当日配送次数
    string assignedOrderID;  // 当前分配的订单编号（"#" 表示无订单）
    string assignedBy;       // 分配此订单的配送员编号
    string currentLocation;  // 当前位置站点代号（A/B/C/D），空闲时停在四个站点之一
    time_t deliveryStartTime;// 配送开始时间戳（用于计算剩余时长，0表示未配送）
    int deliveryTotalMinutes;// 当前配送任务预计总时长（分钟）
    string targetStation;    // 当前配送目标站点代号

public:
    // ---------- 默认构造函数 ----------
    Drone() {
        droneID = "#";
        droneState = Idle;         // 初始状态为空闲
        level = LightDroneLevel;
        maxWeight = LIGHT_MAX_WEIGHT;
        deliveryCount = 0;         // 当日配送次数从0开始
        assignedOrderID = "#";     // "#" 表示未分配订单
        assignedBy = "#";
        currentLocation = "A";     // 初始位置：天马公寓
        deliveryStartTime = 0;     // 未配送
        deliveryTotalMinutes = 0;
        targetStation = "A";
    }

    // ---------- 带参构造函数 ----------
    Drone(string did, DroneState ds, DroneLevel dl, double mw, string loc = "A")
        : droneID(did), droneState(ds), level(dl),
          maxWeight(mw), deliveryCount(0),
          assignedOrderID("#"), assignedBy("#"),
          currentLocation(loc), deliveryStartTime(0),
          deliveryTotalMinutes(0), targetStation(loc) {}

    // ---------- 复制构造函数 ----------
    Drone(const Drone& d)
        : droneID(d.droneID), droneState(d.droneState),
          level(d.level), maxWeight(d.maxWeight),
          deliveryCount(d.deliveryCount),
          assignedOrderID(d.assignedOrderID), assignedBy(d.assignedBy),
          currentLocation(d.currentLocation),
          deliveryStartTime(d.deliveryStartTime),
          deliveryTotalMinutes(d.deliveryTotalMinutes),
          targetStation(d.targetStation) {}

    // ---------- 虚析构函数：确保子类正确析构 ----------
    virtual ~Drone() {}

    // ===== 纯虚函数：子类必须实现 =====
    // 检查该无人机是否能运送指定类型的货物（子类根据等级限制重写）
    virtual bool canCarry(CargoType ct) const = 0;
    // 获取该无人机的特殊功能描述（温控、保险锁等）
    virtual string getSpecialFeatures() const = 0;

    // ===== 虚函数：子类可重写，基类提供默认实现 =====
    // 执行配送过程（接受全局订单列表引用，以便更新订单状态）
    virtual void deliver(vector<Order>& allOrders) {
        if (!hasOrder()) {
            cout << "无人机 " << droneID << " 没有待配送的订单！" << endl;
            return;
        }
        Order* order = NULL;
        for (size_t i = 0; i < allOrders.size(); i++) {
            if (allOrders[i].getOrderID() == assignedOrderID) {
                order = &allOrders[i];
                break;
            }
        }
        if (!order) {
            cout << "错误：找不到订单 " << assignedOrderID << "！" << endl;
            return;
        }
        if (deliveryCount >= MAX_DAILY_DELIVERIES) {
            cout << "无人机 " << droneID << " 当日已达配送上限（"
                 << MAX_DAILY_DELIVERIES << "次），无法继续配送！" << endl;
            assignedOrderID = "#";
            assignedBy = "#";
            order->setOrderState(Pending);
            return;
        }
        // 记录配送起点、目标、开始时间
        string from = order->getFromStation();
        string to   = order->getToStation();
        int minutes = calcDeliveryTime(from, to);
        droneState = DRDelivering;
        targetStation = to;
        deliveryTotalMinutes = minutes;
        deliveryStartTime = time(0);  // 记录开始时间
        order->setOrderState(Delivering);
        cout << "\n=== " << droneLevelToString(level) << "无人机 "
             << droneID << " 正在配送 ===" << endl;
        cout << "起点：" << from << "(" << getStationName(from) << ")"
             << " → 终点：" << to << "(" << getStationName(to) << ")" << endl;
        cout << "订单：" << order->getOrderID() << " | 货物："
             << order->getDescription() << " | 预计 " << minutes << " 分钟" << endl;
        // 模拟配送完成
        order->setOrderState(Delivered);
        deliveryCount++;
        currentLocation = to;          // 无人机到达目标站点
        droneState = Idle;             // 配送完成变为空闲
        assignedOrderID = "#";
        assignedBy = "#";
        deliveryStartTime = 0;
        cout << "无人机 " << droneID << " 已到达 " << getStationName(to)
             << "，配送完成！（" << minutes << "分钟，当日第 "
             << deliveryCount << " 次配送）" << endl;
    }

    // ===== Getter 接口函数 =====
    string getDroneID() const       { return droneID; }
    int getDeliveryCount() const    { return deliveryCount; }
    DroneState getDroneState() const { return droneState; }
    DroneLevel getLevel() const     { return level; }
    double getMaxWeight() const     { return maxWeight; }
    string getAssignedOrderID() const { return assignedOrderID; }
    string getAssignedBy() const    { return assignedBy; }
    string getCurrentLocation() const { return currentLocation; }
    time_t getDeliveryStartTime() const { return deliveryStartTime; }
    int getDeliveryTotalMinutes() const  { return deliveryTotalMinutes; }

    // ===== 计算配送剩余时长（分钟），已完成为0 =====
    int getRemainingMinutes() const {
        if (deliveryStartTime == 0 || droneState != DRDelivering) return 0;
        time_t now = time(0);
        int elapsed = (int)difftime(now, deliveryStartTime) / 60;  // 已过分钟数
        int remain = deliveryTotalMinutes - elapsed;
        return remain > 0 ? remain : 0;
    }

    // ===== Setter 函数（修改无人机的任务绑定） =====
    void setAssignedOrderID(string oid) { assignedOrderID = oid; }
    void setAssignedBy(string ab) { assignedBy = ab; }

    // ---------- 检查是否有分配的订单 ----------
    bool hasOrder() const { return assignedOrderID != "#"; }

    // ---------- 显示无人机完整信息 ----------
    virtual void Show() {
        cout << "\n无人机编号：" << droneID << endl;
        cout << "等级：" << droneLevelToString(level) << endl;
        cout << "最大载重：" << maxWeight << " kg" << endl;
        cout << "特殊功能：" << getSpecialFeatures() << endl;
        cout << "当前位置：" << currentLocation
             << "(" << getStationName(currentLocation) << ")" << endl;
        cout << "当日配送次数：" << deliveryCount << endl;
        cout << "当前状态：" << droneStateToString(droneState) << endl;
        if (hasOrder()) {
            cout << "当前配送订单：" << assignedOrderID << endl;
            cout << "分配配送员：" << assignedBy << endl;
            cout << "配送目标：" << targetStation
                 << "(" << getStationName(targetStation) << ")" << endl;
        } else {
            cout << "当前配送订单：无" << endl;
        }
        // 配送中时显示剩余时长
        if (droneState == DRDelivering) {
            int remain = getRemainingMinutes();
            cout << "剩余配送时间：约 " << remain << " 分钟" << endl;
        }
    }
};

// =====================================================================
// LightDrone 子类 —— 轻型无人机：≤5kg，只能运送普通货物
// =====================================================================
class LightDrone : public Drone {
public:
    // ---------- 默认构造函数：使用轻型无人机参数 ----------
    LightDrone() : Drone("L000", Idle, LightDroneLevel, LIGHT_MAX_WEIGHT) {}

    // ---------- 带参构造函数：可自定义编号和状态 ----------
    LightDrone(string did, DroneState ds)
        : Drone(did, ds, LightDroneLevel, LIGHT_MAX_WEIGHT) {}

    // ---------- 复制构造函数 ----------
    LightDrone(const LightDrone& ld) : Drone(ld) {}

    // ---------- 析构函数 ----------
    ~LightDrone() {}

    // ---------- 轻型只能运送普通货物 ----------
    bool canCarry(CargoType ct) const override {
        return ct == Normal;
    }

    // ---------- 轻型无人机无特殊功能 ----------
    string getSpecialFeatures() const override {
        return "无";
    }

    // ---------- 重写配送函数：加入轻型专属提示 ----------
    void deliver(vector<Order>& allOrders) override {
        if (!hasOrder()) {
            cout << "轻型无人机 " << droneID << " 没有待配送的订单！" << endl;
            return;
        }
        Order* order = NULL;
        for (size_t i = 0; i < allOrders.size(); i++) {
            if (allOrders[i].getOrderID() == assignedOrderID) {
                order = &allOrders[i];
                break;
            }
        }
        if (!order) {
            cout << "错误：找不到订单 " << assignedOrderID << "！" << endl;
            return;
        }
        // 检查是否达到当日配送上限
        if (deliveryCount >= MAX_DAILY_DELIVERIES) {
            cout << "轻型无人机 " << droneID << " 当日已达配送上限（"
                 << MAX_DAILY_DELIVERIES << "次），无法继续配送！" << endl;
            assignedOrderID = "#";
            assignedBy = "#";
            order->setOrderState(Pending);
            return;
        }
        // 轻型配送：记录起点终点和开始时间
        string from = order->getFromStation();
        string to   = order->getToStation();
        int minutes = calcDeliveryTime(from, to);
        droneState = DRDelivering;
        targetStation = to;
        deliveryTotalMinutes = minutes;
        deliveryStartTime = time(0);
        order->setOrderState(Delivering);
        cout << "\n=== 轻型无人机 " << droneID << " 起飞配送 ===" << endl;
        cout << "路线：" << from << "(" << getStationName(from) << ")"
             << " → " << to << "(" << getStationName(to) << ")" << endl;
        cout << "订单：" << order->getOrderID() << " | 货物："
             << order->getDescription() << " | 预计 " << minutes << " 分钟" << endl;
        order->setOrderState(Delivered);
        deliveryCount++;
        currentLocation = to;
        droneState = Idle;
        assignedOrderID = "#";
        assignedBy = "#";
        deliveryStartTime = 0;
        cout << "轻型无人机 " << droneID << " 已到达 " << getStationName(to)
             << "，配送完成！（当日第 " << deliveryCount << " 次）" << endl;
    }

    // ---------- 重写显示函数 ----------
    void Show() override {
        cout << "\n【轻型无人机】";
        Drone::Show();
    }
};

// =====================================================================
// MediumDrone 子类 —— 中型无人机：≤20kg，可运普通/生鲜/药品，带温控
// =====================================================================
class MediumDrone : public Drone {
public:
    // ---------- 默认构造函数 ----------
    MediumDrone() : Drone("M000", Idle, MediumDroneLevel, MEDIUM_MAX_WEIGHT) {}

    // ---------- 带参构造函数 ----------
    MediumDrone(string did, DroneState ds)
        : Drone(did, ds, MediumDroneLevel, MEDIUM_MAX_WEIGHT) {}

    // ---------- 复制构造函数 ----------
    MediumDrone(const MediumDrone& md) : Drone(md) {}

    // ---------- 析构函数 ----------
    ~MediumDrone() {}

    // ---------- 中型可运送：普通货物 + 生鲜 + 药品（不含贵重物品） ----------
    bool canCarry(CargoType ct) const override {
        return ct == Normal || ct == Fresh || ct == Medicine;
    }

    // ---------- 中型无人机特殊功能：温控系统 ----------
    string getSpecialFeatures() const override {
        return "温控系统（恒温 2°C-8°C，保障生鲜/药品品质）";
    }

    // ---------- 重写配送函数：加入温控系统提示 ----------
    void deliver(vector<Order>& allOrders) override {
        if (!hasOrder()) {
            cout << "中型无人机 " << droneID << " 没有待配送的订单！" << endl;
            return;
        }
        Order* order = NULL;
        for (size_t i = 0; i < allOrders.size(); i++) {
            if (allOrders[i].getOrderID() == assignedOrderID) {
                order = &allOrders[i];
                break;
            }
        }
        if (!order) {
            cout << "错误：找不到订单 " << assignedOrderID << "！" << endl;
            return;
        }
        // 检查是否达到当日配送上限
        if (deliveryCount >= MAX_DAILY_DELIVERIES) {
            cout << "中型无人机 " << droneID << " 当日已达配送上限（"
                 << MAX_DAILY_DELIVERIES << "次），无法继续配送！" << endl;
            assignedOrderID = "#";
            assignedBy = "#";
            order->setOrderState(Pending);
            return;
        }
        string from = order->getFromStation();
        string to   = order->getToStation();
        int minutes = calcDeliveryTime(from, to);
        droneState = DRDelivering;
        targetStation = to;
        deliveryTotalMinutes = minutes;
        deliveryStartTime = time(0);
        order->setOrderState(Delivering);
        cout << "\n=== 中型无人机 " << droneID << " 起飞配送 ===" << endl;
        if (order->getCargoType() == Fresh || order->getCargoType() == Medicine) {
            cout << "【温控系统已启动】货舱恒温 2°C-8°C，保障"
                 << cargoTypeToString(order->getCargoType()) << "品质" << endl;
        }
        cout << "路线：" << from << "(" << getStationName(from) << ")"
             << " → " << to << "(" << getStationName(to) << ")" << endl;
        cout << "订单：" << order->getOrderID() << " | 货物："
             << order->getDescription() << " | 预计 " << minutes << " 分钟" << endl;
        order->setOrderState(Delivered);
        deliveryCount++;
        currentLocation = to;
        droneState = Idle;
        assignedOrderID = "#";
        assignedBy = "#";
        deliveryStartTime = 0;
        cout << "中型无人机 " << droneID << " 已到达 " << getStationName(to)
             << "，配送完成！（当日第 " << deliveryCount << " 次）" << endl;
    }

    // ---------- 重写显示函数 ----------
    void Show() override {
        cout << "\n【中型无人机】";
        Drone::Show();
    }
};

// =====================================================================
// HeavyDrone 子类 —— 重型无人机：≤50kg，全品类，温控+保险锁
// =====================================================================
class HeavyDrone : public Drone {
public:
    // ---------- 默认构造函数 ----------
    HeavyDrone() : Drone("H000", Idle, HeavyDroneLevel, HEAVY_MAX_WEIGHT) {}

    // ---------- 带参构造函数 ----------
    HeavyDrone(string did, DroneState ds)
        : Drone(did, ds, HeavyDroneLevel, HEAVY_MAX_WEIGHT) {}

    // ---------- 复制构造函数 ----------
    HeavyDrone(const HeavyDrone& hd) : Drone(hd) {}

    // ---------- 析构函数 ----------
    ~HeavyDrone() {}

    // ---------- 重型可运送所有类型货物 ----------
    bool canCarry(CargoType ct) const override {
        return true;  // 重型无人机无货物类型限制
    }

    // ---------- 重型无人机特殊功能：温控 + 保险锁 ----------
    string getSpecialFeatures() const override {
        return "温控系统 + 保险锁（贵重物品加密锁定，安全运输）";
    }

    // ---------- 重写配送函数：温控 + 保险锁 ----------
    void deliver(vector<Order>& allOrders) override {
        if (!hasOrder()) {
            cout << "重型无人机 " << droneID << " 没有待配送的订单！" << endl;
            return;
        }
        Order* order = NULL;
        for (size_t i = 0; i < allOrders.size(); i++) {
            if (allOrders[i].getOrderID() == assignedOrderID) {
                order = &allOrders[i];
                break;
            }
        }
        if (!order) {
            cout << "错误：找不到订单 " << assignedOrderID << "！" << endl;
            return;
        }
        // 检查是否达到当日配送上限
        if (deliveryCount >= MAX_DAILY_DELIVERIES) {
            cout << "重型无人机 " << droneID << " 当日已达配送上限（"
                 << MAX_DAILY_DELIVERIES << "次），无法继续配送！" << endl;
            assignedOrderID = "#";
            assignedBy = "#";
            order->setOrderState(Pending);
            return;
        }
        string from = order->getFromStation();
        string to   = order->getToStation();
        int minutes = calcDeliveryTime(from, to);
        droneState = DRDelivering;
        targetStation = to;
        deliveryTotalMinutes = minutes;
        deliveryStartTime = time(0);
        order->setOrderState(Delivering);
        cout << "\n=== 重型无人机 " << droneID << " 起飞配送 ===" << endl;
        if (order->getCargoType() == Fresh || order->getCargoType() == Medicine) {
            cout << "【温控系统已启动】货舱恒温 2°C-8°C" << endl;
        }
        if (order->getCargoType() == Valuable) {
            cout << "【保险锁已激活】贵重物品已加密锁定，需收件人身份验证解锁" << endl;
        }
        cout << "路线：" << from << "(" << getStationName(from) << ")"
             << " → " << to << "(" << getStationName(to) << ")" << endl;
        cout << "订单：" << order->getOrderID() << " | 货物："
             << order->getDescription() << " | 预计 " << minutes << " 分钟" << endl;
        order->setOrderState(Delivered);
        deliveryCount++;
        currentLocation = to;
        droneState = Idle;
        assignedOrderID = "#";
        assignedBy = "#";
        deliveryStartTime = 0;
        if (order->getCargoType() == Valuable) {
            cout << "【保险锁已解除】贵重物品安全送达" << endl;
        }
        cout << "重型无人机 " << droneID << " 已到达 " << getStationName(to)
             << "，配送完成！（当日第 " << deliveryCount << " 次）" << endl;
    }

    // ---------- 重写显示函数 ----------
    void Show() override {
        cout << "\n【重型无人机】";
        Drone::Show();
    }
};

// =====================================================================
// 全局变量：三种等级的无人机容器（仿照原始代码的 srobot/brobot）
// =====================================================================
vector<LightDrone>  lightDrones;   // 轻型无人机列表
vector<MediumDrone> mediumDrones;  // 中型无人机列表
vector<HeavyDrone>  heavyDrones;   // 重型无人机列表

// =====================================================================
// 预置数据初始化：创建初始无人机和示例订单
// =====================================================================
void initPresetData() {
    // ---- 添加预置轻型无人机（L001, L002） ----
    LightDrone ld1("L001", Idle);
    LightDrone ld2("L002", Idle);
    lightDrones.push_back(ld1);
    lightDrones.push_back(ld2);

    // ---- 添加预置中型无人机（M001, M002） ----
    MediumDrone md1("M001", Idle);
    MediumDrone md2("M002", Idle);
    mediumDrones.push_back(md1);
    mediumDrones.push_back(md2);

    // ---- 添加预置重型无人机（H001） ----
    HeavyDrone hd1("H001", Idle);
    heavyDrones.push_back(hd1);

    // ---- 添加预设示例订单，方便演示各角色功能 ----
    // (订单号, 类型, 姓名, 电话, 地址, 重量, 描述, 状态, 用户, 预约时间, 时段, 起点, 终点, 预计时长)
    Order o1(generateOrderID(), Normal,   "张三", "13800001111", "天马公寓3栋301",  3.0,  "书籍-《C++程序设计》", Pending, "user001", 0, "", "A", "B", calcDeliveryTime("A","B"));
    Order o2(generateOrderID(), Fresh,    "李四", "13800002222", "德智公寓2栋205",  8.0,  "生鲜-新鲜水果拼盘",    Pending, "user002", 0, "", "B", "C", calcDeliveryTime("B","C"));
    Order o3(generateOrderID(), Medicine, "王五", "13800003333", "综合楼2楼药房",    2.5,  "药品-感冒灵颗粒",      Pending, "user001", 0, "", "A", "C", calcDeliveryTime("A","C"));
    Order o4(generateOrderID(), Valuable, "赵六", "13800004444", "东方红广场12号",   1.0,  "贵重-笔记本电脑",      Pending, "user003", 0, "", "C", "D", calcDeliveryTime("C","D"));
    Order o5(generateOrderID(), Normal,   "张三", "13800001111", "天马公寓3栋301",  4.5,  "日用品-洗衣液套装",    Pending, "user001", 0, "", "A", "A", 0);
    allOrders.push_back(o1);
    allOrders.push_back(o2);
    allOrders.push_back(o3);
    allOrders.push_back(o4);
    allOrders.push_back(o5);
}

// =====================================================================
// 账户结构：存储预置用户的账号、密码和角色信息
// =====================================================================
struct Account {
    string username;           // 登录账号
    string password;           // 登录密码（明文存储，不加密）
    string role;               // 角色类型："admin"管理员 / "deliver"配送员 / "user"普通用户
    string displayName;        // 显示名称
    string idCard;             // 身份证号（下单前验证用）
    string phone;              // 电话号码
    int cancelCount = 0;       // 本周取消订单次数
    time_t cancelWeekStart = 0;// 本周起始时间戳（用于判断是否跨周重置）
    time_t bannedUntil = 0;    // 封禁截止时间戳（0表示未被封禁）
};

// =====================================================================
// 检查账号是否处于封禁状态（bannedUntil > 当前时间 则被封禁）
// =====================================================================
bool isBanned(const Account& acc) {
    if (acc.bannedUntil == 0) return false;    // 从未被封禁
    time_t now = time(0);                      // 获取当前时间
    return now < acc.bannedUntil;              // 封禁未到期则返回true
}

// =====================================================================
// 获取封禁剩余天数（用于向用户展示），已解封返回0
// =====================================================================
int getBanRemainingDays(const Account& acc) {
    if (!isBanned(acc)) return 0;
    time_t now = time(0);
    double diff = difftime(acc.bannedUntil, now);  // 计算剩余秒数
    int days = (int)(diff / (60 * 60 * 24)) + 1;   // 向上取整到天
    return days > 0 ? days : 1;
}

// =====================================================================
// 将封禁截止时间戳格式化为日期字符串
// =====================================================================
string formatBanTime(time_t t) {
    if (t == 0) return "未封禁";
    tm* btm = localtime(&t);
    stringstream ss;
    ss << (1900 + btm->tm_year) << "-"
       << setw(2) << setfill('0') << (1 + btm->tm_mon) << "-"
       << setw(2) << setfill('0') << btm->tm_mday;
    return ss.str();
}

// =====================================================================
// 重置本周取消次数（如果已进入新的一周）
// =====================================================================
void resetCancelCountIfNewWeek(Account& acc) {
    time_t now = time(0);
    tm* nowTm = localtime(&now);
    // 计算本周一 00:00:00 的时间戳
    tm weekStartTm = *nowTm;
    weekStartTm.tm_hour = 0; weekStartTm.tm_min = 0; weekStartTm.tm_sec = 0;
    // tm_wday: 0=周日, 1=周一... 本周一 = 当前时间 - (wday-1)天（周日特殊处理）
    int daysFromMonday = nowTm->tm_wday == 0 ? 6 : nowTm->tm_wday - 1;
    weekStartTm.tm_mday -= daysFromMonday;
    time_t thisMonday = mktime(&weekStartTm);   // 本周一0点的时间戳
    // 如果记录的周起始时间早于本周一，说明进入了新的一周，重置计数
    if (acc.cancelWeekStart < thisMonday) {
        acc.cancelCount = 0;
        acc.cancelWeekStart = thisMonday;
    }
}

// =====================================================================
// 记录一次取消订单操作：取消次数+1，达到3次则封号2周
// =====================================================================
void recordCancellation(Account& acc) {
    resetCancelCountIfNewWeek(acc);               // 先检查是否需要跨周重置
    acc.cancelCount++;                            // 取消次数+1
    cout << "（本周已取消 " << acc.cancelCount << " 次）" << endl;
    if (acc.cancelCount >= 3) {
        time_t now = time(0);
        acc.bannedUntil = now + 14 * 24 * 60 * 60;// 封禁2周（14天）
        cout << "⚠ 您的本周取消次数已达3次，账号已被封禁至 "
             << formatBanTime(acc.bannedUntil) << "！" << endl;
    }
}

// =====================================================================
// 管理员手动解封账号
// =====================================================================
void adminUnbanAccount(Account& acc) {
    acc.bannedUntil = 0;      // 清除封禁时间
    acc.cancelCount = 0;      // 重置取消次数
    time_t now = time(0);
    tm* nowTm = localtime(&now);
    tm weekStartTm = *nowTm;
    weekStartTm.tm_hour = 0; weekStartTm.tm_min = 0; weekStartTm.tm_sec = 0;
    int daysFromMonday = nowTm->tm_wday == 0 ? 6 : nowTm->tm_wday - 1;
    weekStartTm.tm_mday -= daysFromMonday;
    acc.cancelWeekStart = mktime(&weekStartTm);   // 重置周起始时间
    cout << "账号 " << acc.username << " 已手动解封！" << endl;
}

// =====================================================================
// 全局变量：预置账户列表
// =====================================================================
vector<Account> accounts;

// =====================================================================
// 初始化预置账户：管理员、配送员、普通用户的账号密码
// =====================================================================
void initAccounts() {
    // ---- 管理员（1个），账号：admin，密码：admin123 ----
    accounts.push_back({"admin", "admin123", "admin", "管理员", "", ""});
    // ---- 配送员（2个），账号：delivery001/delivery002，密码：del001/del002 ----
    accounts.push_back({"delivery001", "del001", "deliver", "配送员delivery001", "", ""});
    accounts.push_back({"delivery002", "del002", "deliver", "配送员delivery002", "", ""});
    // ---- 普通用户（3个），账号：user001/user002/user003，密码：123456 ----
    accounts.push_back({"user001", "123456", "user", "张三", "110101199001011234", "13800001111"});
    accounts.push_back({"user002", "123456", "user", "李四", "110101199002022345", "13800002222"});
    accounts.push_back({"user003", "123456", "user", "赵六", "110101199003033456", "13800003333"});
}

// =====================================================================
// 通用登录函数：根据角色类型验证账号密码
// 参数 role："admin"/"deliver"/"user"，筛选对应角色的账户
// 返回值：登录成功返回指向该 Account 的指针，失败返回 NULL
// =====================================================================
Account* login(const string& role) {
    string username, password;
    cout << "请输入账号: ";
    cin >> username;
    cout << "请输入密码: ";
    cin >> password;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');  // 清除输入缓冲区

    // 遍历所有账户，匹配账号、密码和角色
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].username == username
            && accounts[i].password == password
            && accounts[i].role == role) {
            return &accounts[i];  // 登录成功，返回账户指针
        }
    }
    return NULL;  // 登录失败：账号或密码错误，或角色不匹配
}

// =====================================================================
// 管理员菜单：显示管理员可执行的操作
// =====================================================================
void displayAdminMenu() {
    cout << "\n=========== 管理员菜单 ===========" << endl;
    cout << "1. 添加无人机" << endl;
    cout << "2. 查看所有无人机" << endl;
    cout << "3. 查看所有订单" << endl;
    cout << "4. 查看统计报告" << endl;
    cout << "5. 查看当日无人机配送记录" << endl;
    cout << "6. 用户封禁管理" << endl;
    cout << "0. 返回主菜单" << endl;
    cout << "===================================" << endl;
    cout << "请选择操作 (0-6): ";
}

// =====================================================================
// 管理员功能1：添加无人机（选择等级和编号）
// =====================================================================
void adminAddDrone() {
    clearScreen();
    cout << "\n=========== 添加无人机 ===========" << endl;
    // 让管理员选择无人机等级
    cout << "请选择无人机等级：" << endl;
    cout << "  1. 轻型（载重≤" << LIGHT_MAX_WEIGHT << "kg，仅普通货物）" << endl;
    cout << "  2. 中型（载重≤" << MEDIUM_MAX_WEIGHT << "kg，普通+生鲜+药品，温控）" << endl;
    cout << "  3. 重型（载重≤" << HEAVY_MAX_WEIGHT << "kg，全品类，温控+保险锁）" << endl;
    cout << "请选择 (1-3): ";
    int levelChoice;
    levelChoice = readInt();

    // 输入无人机编号
    string newID;
    string prefix;  // 编号前缀：L/M/H
    switch (levelChoice) {
        case 1: prefix = "L"; break;
        case 2: prefix = "M"; break;
        case 3: prefix = "H"; break;
        default:
            cout << "无效选择！" << endl;
            return;
    }
    cout << "请输入无人机编号（" << prefix << "+3位数字，如 " << prefix << "003）: ";
    cin >> newID;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    // 检查编号是否重复：遍历对应等级的无人机列表
    bool duplicate = false;
    if (levelChoice == 1) {
        for (size_t i = 0; i < lightDrones.size(); i++) {
            if (lightDrones[i].getDroneID() == newID) { duplicate = true; break; }
        }
        if (!duplicate) {
            LightDrone newDrone(newID, Idle);
            lightDrones.push_back(newDrone);
            cout << "轻型无人机 " << newID << " 添加成功！" << endl;
        }
    } else if (levelChoice == 2) {
        for (size_t i = 0; i < mediumDrones.size(); i++) {
            if (mediumDrones[i].getDroneID() == newID) { duplicate = true; break; }
        }
        if (!duplicate) {
            MediumDrone newDrone(newID, Idle);
            mediumDrones.push_back(newDrone);
            cout << "中型无人机 " << newID << " 添加成功！" << endl;
        }
    } else if (levelChoice == 3) {
        for (size_t i = 0; i < heavyDrones.size(); i++) {
            if (heavyDrones[i].getDroneID() == newID) { duplicate = true; break; }
        }
        if (!duplicate) {
            HeavyDrone newDrone(newID, Idle);
            heavyDrones.push_back(newDrone);
            cout << "重型无人机 " << newID << " 添加成功！" << endl;
        }
    }
    if (duplicate) {
        cout << "错误：无人机 " << newID << " 已存在！" << endl;
    }
    waitForReturn();
}

// =====================================================================
// 管理员功能2：查看所有无人机信息
// =====================================================================
void adminShowAllDrones() {
    clearScreen();
    cout << "\n=========== 所有无人机信息 ===========" << endl;
    // 显示轻型无人机
    if (lightDrones.empty()) {
        cout << "\n【轻型无人机】暂无" << endl;
    } else {
        for (size_t i = 0; i < lightDrones.size(); i++) {
            lightDrones[i].Show();
        }
    }
    // 显示中型无人机
    if (mediumDrones.empty()) {
        cout << "\n【中型无人机】暂无" << endl;
    } else {
        for (size_t i = 0; i < mediumDrones.size(); i++) {
            mediumDrones[i].Show();
        }
    }
    // 显示重型无人机
    if (heavyDrones.empty()) {
        cout << "\n【重型无人机】暂无" << endl;
    } else {
        for (size_t i = 0; i < heavyDrones.size(); i++) {
            heavyDrones[i].Show();
        }
    }
    cout << "\n总计：轻型 " << lightDrones.size()
         << " 架 | 中型 " << mediumDrones.size()
         << " 架 | 重型 " << heavyDrones.size() << " 架" << endl;
    waitForReturn();
}

// =====================================================================
// 管理员功能3：查看所有订单信息
// =====================================================================
void adminShowAllOrders() {
    clearScreen();
    cout << "\n=========== 所有订单信息 ===========" << endl;
    if (allOrders.empty()) {
        cout << "暂无订单！" << endl;
    } else {
        for (size_t i = 0; i < allOrders.size(); i++) {
            cout << "\n--- 订单 #" << (i + 1) << " ---" << endl;
            allOrders[i].Show();
        }
        cout << "\n订单总数：" << allOrders.size() << endl;
    }
    waitForReturn();
}

// =====================================================================
// 管理员功能4：查看统计报告（各状态/类型订单数、无人机使用率）
// =====================================================================
void adminShowReport() {
    clearScreen();
    cout << "\n=========== 系统统计报告 ===========" << endl;

    // ---- 统计各状态订单数 ----
    int cntPending = 0, cntAssigned = 0, cntDelivering = 0;
    int cntDelivered = 0, cntReceived = 0, cntCancelled = 0;
    for (size_t i = 0; i < allOrders.size(); i++) {
        switch (allOrders[i].getOrderState()) {
            case Pending:    cntPending++;    break;
            case Assigned:   cntAssigned++;   break;
            case Delivering: cntDelivering++; break;
            case Delivered:  cntDelivered++;  break;
            case Received:   cntReceived++;   break;
            case Cancelled:  cntCancelled++;  break;
        }
    }
    cout << "\n--- 订单状态统计 ---" << endl;
    cout << "待处理：" << cntPending    << " | 已分配：" << cntAssigned
         << " | 配送中：" << cntDelivering << endl;
    cout << "已送达：" << cntDelivered  << " | 已收货：" << cntReceived
         << " | 已取消：" << cntCancelled
         << " | 合计：" << allOrders.size() << endl;

    // ---- 统计各类型订单数 ----
    int cntNormal = 0, cntFresh = 0, cntMedicine = 0, cntValuable = 0;
    for (size_t i = 0; i < allOrders.size(); i++) {
        switch (allOrders[i].getCargoType()) {
            case Normal:   cntNormal++;   break;
            case Fresh:    cntFresh++;    break;
            case Medicine: cntMedicine++; break;
            case Valuable: cntValuable++; break;
        }
    }
    cout << "\n--- 货物类型统计 ---" << endl;
    cout << "普通货物：" << cntNormal   << " | 生鲜：" << cntFresh
         << " | 药品：" << cntMedicine << " | 贵重物品：" << cntValuable << endl;

    // ---- 统计无人机使用率（有空闲/总数为正在使用） ----
    int totalDrones = lightDrones.size() + mediumDrones.size() + heavyDrones.size();
    int busyDrones = 0;  // 非空闲的无人机数量
    for (size_t i = 0; i < lightDrones.size(); i++) {
        if (lightDrones[i].getDroneState() != Idle) busyDrones++;
    }
    for (size_t i = 0; i < mediumDrones.size(); i++) {
        if (mediumDrones[i].getDroneState() != Idle) busyDrones++;
    }
    for (size_t i = 0; i < heavyDrones.size(); i++) {
        if (heavyDrones[i].getDroneState() != Idle) busyDrones++;
    }
    cout << "\n--- 无人机使用情况 ---" << endl;
    cout << "总数：" << totalDrones << " 架 | 使用中：" << busyDrones
         << " 架 | 空闲：" << (totalDrones - busyDrones) << " 架" << endl;
    waitForReturn();
}

// =====================================================================
// 管理员功能5：查看每架无人机的配送次数 + 已完成订单列表
// =====================================================================
void adminShowDailyDeliveries() {
    clearScreen();
    cout << "\n=========== 当日无人机配送记录 ===========" << endl;

    // ---- 第一步：列出每架无人机的配送次数 ----
    int totalDeliveries = 0;  // 所有无人机的总配送次数
    auto showDrones = [&](const string& label, auto& drones) {
        if (!drones.empty()) {
            cout << "\n--- " << label << " ---" << endl;
            for (size_t i = 0; i < drones.size(); i++) {
                int cnt = drones[i].getDeliveryCount();
                totalDeliveries += cnt;
                cout << "  " << drones[i].getDroneID()
                     << "：当日配送 " << cnt << " / " << MAX_DAILY_DELIVERIES << " 次"
                     << "  |  当前位置：" << drones[i].getCurrentLocation()
                     << "(" << getStationName(drones[i].getCurrentLocation()) << ")";
                if (cnt >= MAX_DAILY_DELIVERIES) cout << "（已达上限）";
                cout << endl;
            }
        }
    };
    showDrones("轻型无人机", lightDrones);
    showDrones("中型无人机", mediumDrones);
    showDrones("重型无人机", heavyDrones);
    cout << "\n当日总配送次数：" << totalDeliveries << endl;

    // ---- 第二步：列出所有已完成的订单（已送达 + 已收货） ----
    cout << "\n--- 已完成订单列表 ---" << endl;
    bool hasOrders = false;
    for (size_t i = 0; i < allOrders.size(); i++) {
        OrderState os = allOrders[i].getOrderState();
        if (os == Delivered || os == Received) {
            hasOrders = true;
            cout << "  " << allOrders[i].getOrderID()
                 << " | " << cargoTypeToString(allOrders[i].getCargoType())
                 << " | " << allOrders[i].getDescription()
                 << " | " << allOrders[i].getFromStation()
                 << "→" << allOrders[i].getToStation()
                 << "(" << allOrders[i].getDeliveryMinutes() << "min)"
                 << " | 状态：" << orderStateToString(os) << endl;
        }
    }
    if (!hasOrders) cout << "  暂无已完成的订单" << endl;
    waitForReturn();
}

// =====================================================================
// 管理员功能6：查看用户取消次数和封禁状态，并可手动解封
// =====================================================================
void adminManageUsers() {
    clearScreen();
    cout << "\n=========== 用户封禁管理 ===========" << endl;

    // 列出所有普通用户及其状态
    cout << "\n--- 普通用户状态 ---" << endl;
    int userCount = 0;
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].role == "user") {
            // 跨周检查：先刷新取消次数
            resetCancelCountIfNewWeek(accounts[i]);
            bool banned = isBanned(accounts[i]);
            cout << userCount + 1 << ". 账号：" << accounts[i].username
                 << "（" << accounts[i].displayName << "）" << endl;
            cout << "   本周取消次数：" << accounts[i].cancelCount << " / 3" << endl;
            if (banned) {
                cout << "   状态：已封禁（至 " << formatBanTime(accounts[i].bannedUntil)
                     << "，剩余 " << getBanRemainingDays(accounts[i]) << " 天）" << endl;
            } else {
                cout << "   状态：正常" << endl;
            }
            userCount++;
        }
    }

    if (userCount == 0) {
        cout << "暂无用户！" << endl;
        waitForReturn();
        return;
    }

    // 提供解封选项
    cout << "\n输入要解封的用户账号（输入0返回）: ";
    string uname;
    cin >> uname;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    if (uname == "0") return;

    // 查找并解封
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].username == uname && accounts[i].role == "user") {
            if (!isBanned(accounts[i])) {
                cout << "该用户未被封禁！" << endl;
                waitForReturn();
                return;
            }
            adminUnbanAccount(accounts[i]);
            waitForReturn();
            return;
        }
    }
    cout << "未找到用户 " << uname << "！" << endl;
    waitForReturn();
}

// =====================================================================
// 管理员主循环：接收管理员菜单选择并分发到对应功能
// =====================================================================
void adminLoop() {
    int choice;
    do {
        clearScreen();
        displayAdminMenu();
        choice = readInt();
        switch (choice) {
            case 1: adminAddDrone();              break;  // 添加无人机
            case 2: adminShowAllDrones();         break;  // 查看所有无人机
            case 3: adminShowAllOrders();         break;  // 查看所有订单
            case 4: adminShowReport();            break;  // 查看统计报告
            case 5: adminShowDailyDeliveries();   break;  // 查看当日配送记录
            case 6: adminManageUsers();           break;  // 用户封禁管理
            case 0: cout << "返回主菜单..." << endl; break;
            default: cout << "无效选项，请重新选择！" << endl; waitForReturn();
        }
    } while (choice != 0);
}

// =====================================================================
// 配送员菜单：显示配送员可执行的操作
// =====================================================================
void displayDeliverMenu(string delivererID) {
    cout << "\n=========== 配送员菜单 [" << delivererID << "] ===========" << endl;
    cout << "1. 查看待处理订单" << endl;
    cout << "2. 分配订单给无人机" << endl;
    cout << "3. 执行配送" << endl;
    cout << "4. 查看已完成订单" << endl;
    cout << "0. 返回主菜单" << endl;
    cout << "=====================================================" << endl;
    cout << "请选择操作 (0-4): ";
}

// =====================================================================
// 配送员功能1：查看所有待处理订单（状态为 Pending 的订单）
// =====================================================================
void deliverShowPendingOrders() {
    clearScreen();
    cout << "\n=========== 待处理订单列表 ===========" << endl;
    bool found = false;
    for (size_t i = 0; i < allOrders.size(); i++) {
        if (allOrders[i].getOrderState() == Pending) {
            cout << "\n--- 待处理订单 #" << (i + 1) << " ---" << endl;
            allOrders[i].Show();
            found = true;
        }
    }
    if (!found) {
        cout << "当前没有待处理的订单！" << endl;
    }
    waitForReturn();
}

// =====================================================================
// 配送员功能2：分配待处理订单给空闲无人机
// 逻辑：选择订单 → 选择空闲且能承运该货物的无人机 → 检查载重 → 绑定
// =====================================================================
void deliverAssignOrder(string delivererID) {
    clearScreen();
    cout << "\n=========== 分配订单给无人机 ===========" << endl;

    // 第一步：列出所有待处理的订单，让配送员选择
    cout << "\n--- 待处理订单 ---" << endl;
    int pendingCount = 0;  // 待处理订单计数
    vector<int> pendingIndices;  // 记录待处理订单在 allOrders 中的索引
    for (size_t i = 0; i < allOrders.size(); i++) {
        if (allOrders[i].getOrderState() == Pending) {
            cout << pendingCount + 1 << ". 订单 " << allOrders[i].getOrderID()
                 << " | " << cargoTypeToString(allOrders[i].getCargoType())
                 << " | " << allOrders[i].getWeight() << "kg"
                 << " | " << allOrders[i].getRecipientAddress() << endl;
            pendingIndices.push_back(i);
            pendingCount++;
        }
    }
    if (pendingCount == 0) {
        cout << "没有待处理的订单！" << endl;
        waitForReturn();
        return;
    }

    // 选择订单
    cout << "请选择要分配的订单 (1-" << pendingCount << "): ";
    int orderChoice;
    orderChoice = readInt();
    if (orderChoice < 1 || orderChoice > pendingCount) {
        cout << "无效选择！" << endl;
        waitForReturn();
        return;
    }
    int orderIndex = pendingIndices[orderChoice - 1];  // 获取订单在 allOrders 中的真实索引
    Order& selectedOrder = allOrders[orderIndex];

    // 第二步：列出所有空闲且能承运该类型货物的无人机
    cout << "\n--- 可用的空闲无人机 ---" << endl;
    int droneCount = 0;  // 可用无人机计数
    // 用一个结构存储无人机信息以便选择
    struct DroneOption {
        int droneType;   // 0=轻型, 1=中型, 2=重型
        int droneIndex;  // 在对应 vector 中的索引
    };
    vector<DroneOption> options;  // 可用无人机选项列表

    // 检查轻型无人机（必须空闲、能承运、不超重、未达当日上限）
    for (size_t i = 0; i < lightDrones.size(); i++) {
        if (lightDrones[i].getDroneState() == Idle
            && lightDrones[i].canCarry(selectedOrder.getCargoType())
            && selectedOrder.getWeight() <= lightDrones[i].getMaxWeight()
            && lightDrones[i].getDeliveryCount() < MAX_DAILY_DELIVERIES) {
            DroneOption opt = {0, (int)i};
            options.push_back(opt);
        }
    }
    // 检查中型无人机（必须空闲、能承运、不超重、未达当日上限）
    for (size_t i = 0; i < mediumDrones.size(); i++) {
        if (mediumDrones[i].getDroneState() == Idle
            && mediumDrones[i].canCarry(selectedOrder.getCargoType())
            && selectedOrder.getWeight() <= mediumDrones[i].getMaxWeight()
            && mediumDrones[i].getDeliveryCount() < MAX_DAILY_DELIVERIES) {
            DroneOption opt = {1, (int)i};
            options.push_back(opt);
        }
    }
    // 检查重型无人机（必须空闲、能承运、不超重、未达当日上限）
    for (size_t i = 0; i < heavyDrones.size(); i++) {
        if (heavyDrones[i].getDroneState() == Idle
            && heavyDrones[i].canCarry(selectedOrder.getCargoType())
            && selectedOrder.getWeight() <= heavyDrones[i].getMaxWeight()
            && heavyDrones[i].getDeliveryCount() < MAX_DAILY_DELIVERIES) {
            DroneOption opt = {2, (int)i};
            options.push_back(opt);
        }
    }

    // 显示可用无人机列表
    for (size_t i = 0; i < options.size(); i++) {
        string droneID;
        double maxW;
        string features;
        int delCnt;  // 当日已配送次数
        if (options[i].droneType == 0) {
            droneID = lightDrones[options[i].droneIndex].getDroneID();
            maxW = lightDrones[options[i].droneIndex].getMaxWeight();
            features = lightDrones[options[i].droneIndex].getSpecialFeatures();
            delCnt = lightDrones[options[i].droneIndex].getDeliveryCount();
        } else if (options[i].droneType == 1) {
            droneID = mediumDrones[options[i].droneIndex].getDroneID();
            maxW = mediumDrones[options[i].droneIndex].getMaxWeight();
            features = mediumDrones[options[i].droneIndex].getSpecialFeatures();
            delCnt = mediumDrones[options[i].droneIndex].getDeliveryCount();
        } else {
            droneID = heavyDrones[options[i].droneIndex].getDroneID();
            maxW = heavyDrones[options[i].droneIndex].getMaxWeight();
            features = heavyDrones[options[i].droneIndex].getSpecialFeatures();
            delCnt = heavyDrones[options[i].droneIndex].getDeliveryCount();
        }
        cout << i + 1 << ". " << droneID << " | 载重≤" << maxW
             << "kg | 已配送" << delCnt << "/" << MAX_DAILY_DELIVERIES
             << "次 | " << features << endl;
    }

    if (options.empty()) {
        cout << "没有可用的无人机！可能原因：无人机已全部占用/已达当日配送上限/载重不够/等级不支持该货物类型。" << endl;
        waitForReturn();
        return;
    }

    // 选择无人机
    cout << "请选择无人机 (1-" << options.size() << "): ";
    int droneChoice;
    droneChoice = readInt();
    if (droneChoice < 1 || droneChoice > (int)options.size()) {
        cout << "无效选择！" << endl;
        waitForReturn();
        return;
    }

    // 第三步：将订单分配给选中的无人机，更新订单状态
    DroneOption chosen = options[droneChoice - 1];
    if (chosen.droneType == 0) {
        lightDrones[chosen.droneIndex].setAssignedOrderID(selectedOrder.getOrderID());
        lightDrones[chosen.droneIndex].setAssignedBy(delivererID);
    } else if (chosen.droneType == 1) {
        mediumDrones[chosen.droneIndex].setAssignedOrderID(selectedOrder.getOrderID());
        mediumDrones[chosen.droneIndex].setAssignedBy(delivererID);
    } else {
        heavyDrones[chosen.droneIndex].setAssignedOrderID(selectedOrder.getOrderID());
        heavyDrones[chosen.droneIndex].setAssignedBy(delivererID);
    }
    selectedOrder.setOrderState(Assigned);  // 订单状态变为"已分配"
    cout << "\n订单 " << selectedOrder.getOrderID() << " 已分配给无人机，等待配送！" << endl;
    waitForReturn();
}

// =====================================================================
// 配送员功能3：执行配送（选择已分配订单的无人机，模拟配送过程）
// =====================================================================
void deliverExecute(string delivererID) {
    clearScreen();
    cout << "\n=========== 执行配送 ===========" << endl;

    // 列出已分配订单的无人机（只显示当前配送员分配的）
    struct DeliveryTask {
        int droneType;
        int droneIndex;
    };
    vector<DeliveryTask> tasks;

    // 收集轻型无人机的配送任务
    for (size_t i = 0; i < lightDrones.size(); i++) {
        if (lightDrones[i].hasOrder() && lightDrones[i].getAssignedBy() == delivererID) {
            DeliveryTask t = {0, (int)i};
            tasks.push_back(t);
        }
    }
    // 收集中型无人机的配送任务
    for (size_t i = 0; i < mediumDrones.size(); i++) {
        if (mediumDrones[i].hasOrder() && mediumDrones[i].getAssignedBy() == delivererID) {
            DeliveryTask t = {1, (int)i};
            tasks.push_back(t);
        }
    }
    // 收集重型无人机的配送任务
    for (size_t i = 0; i < heavyDrones.size(); i++) {
        if (heavyDrones[i].hasOrder() && heavyDrones[i].getAssignedBy() == delivererID) {
            DeliveryTask t = {2, (int)i};
            tasks.push_back(t);
        }
    }

    if (tasks.empty()) {
        cout << "您当前没有待配送的任务！请先分配订单给无人机。" << endl;
        waitForReturn();
        return;
    }

    // 显示待配送任务列表
    cout << "\n--- 待配送任务 ---" << endl;
    for (size_t i = 0; i < tasks.size(); i++) {
        string droneID, orderID;
        if (tasks[i].droneType == 0) {
            droneID = lightDrones[tasks[i].droneIndex].getDroneID();
            orderID = lightDrones[tasks[i].droneIndex].getAssignedOrderID();
        } else if (tasks[i].droneType == 1) {
            droneID = mediumDrones[tasks[i].droneIndex].getDroneID();
            orderID = mediumDrones[tasks[i].droneIndex].getAssignedOrderID();
        } else {
            droneID = heavyDrones[tasks[i].droneIndex].getDroneID();
            orderID = heavyDrones[tasks[i].droneIndex].getAssignedOrderID();
        }
        cout << i + 1 << ". 无人机 " << droneID << " → 订单 " << orderID << endl;
    }

    // 选择要执行的配送任务
    cout << "请选择要执行的配送任务 (1-" << tasks.size() << "): ";
    int choice;
    choice = readInt();
    if (choice < 1 || choice > (int)tasks.size()) {
        cout << "无效选择！" << endl;
        waitForReturn();
        return;
    }

    // 执行配送：调用对应无人机的 deliver() 方法
    DeliveryTask chosen = tasks[choice - 1];
    if (chosen.droneType == 0) {
        lightDrones[chosen.droneIndex].deliver(allOrders);
    } else if (chosen.droneType == 1) {
        mediumDrones[chosen.droneIndex].deliver(allOrders);
    } else {
        heavyDrones[chosen.droneIndex].deliver(allOrders);
    }
    waitForReturn();
}

// =====================================================================
// 配送员功能4：查看所有已完成订单（已分配/配送中/已送达/已收货）
// =====================================================================
void deliverShowRecords(string delivererID) {
    clearScreen();
    cout << "\n=========== 已完成订单记录 ===========" << endl;
    bool found = false;
    for (size_t i = 0; i < allOrders.size(); i++) {
        // 显示所有非待处理状态的订单（即已被配送员处理过的）
        if (allOrders[i].getOrderState() != Pending) {
            cout << "\n--- 配送记录 #" << (i + 1) << " ---" << endl;
            allOrders[i].Show();
            found = true;
        }
    }
    if (!found) {
        cout << "暂无配送记录！" << endl;
    }
    waitForReturn();
}

// =====================================================================
// 配送员主循环：接收菜单选择并分发到对应功能
// =====================================================================
void deliverLoop(string delivererID) {
    int choice;
    do {
        clearScreen();
        displayDeliverMenu(delivererID);
        choice = readInt();
        switch (choice) {
            case 1: deliverShowPendingOrders();        break;  // 查看待处理订单
            case 2: deliverAssignOrder(delivererID);   break;  // 分配订单给无人机
            case 3: deliverExecute(delivererID);       break;  // 执行配送
            case 4: deliverShowRecords(delivererID);   break;  // 查看已完成订单
            case 0: cout << "返回主菜单..." << endl;    break;
            default: cout << "无效选项，请重新选择！" << endl; waitForReturn();
        }
    } while (choice != 0);
}

// =====================================================================
// 普通用户菜单：显示用户可执行的操作
// =====================================================================
void displayUserMenu(string userName) {
    cout << "\n=========== 用户菜单 [" << userName << "] ===========" << endl;
    cout << "1. 下单（即日/预约）" << endl;
    cout << "2. 查看我的订单" << endl;
    cout << "3. 确认收货" << endl;
    cout << "4. 取消订单" << endl;
    cout << "0. 返回主菜单" << endl;
    cout << "===================================================" << endl;
    cout << "请选择操作 (0-4): ";
}

// =====================================================================
// 用户功能1：下单（可选择即日配送或3天内预约配送，选择时段）
// =====================================================================
void userPlaceOrder(string userName) {
    clearScreen();
    cout << "\n=========== 下单 ===========" << endl;

    // 先从 accounts 中找到当前用户
    Account* myAcc = NULL;
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].username == userName) {
            myAcc = &accounts[i];
            break;
        }
    }
    if (!myAcc) { cout << "错误：找不到账户！" << endl; waitForReturn(); return; }

    // 检查是否被封禁（被封禁用户不能下单）
    if (isBanned(*myAcc)) {
        cout << "\n⚠ 您的账号已被封禁至 " << formatBanTime(myAcc->bannedUntil)
             << "（剩余 " << getBanRemainingDays(*myAcc) << " 天），无法下单！" << endl;
        waitForReturn();
        return;
    }

    // 身份证验证
    cout << "\n请输入身份证号验证身份: ";
    string inputId;
    cin >> inputId;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    if (inputId != myAcc->idCard) {
        cout << "身份证号错误！下单失败。" << endl;
        waitForReturn();
        return;
    }
    cout << "身份验证通过！" << endl;

    // 选择货物类型
    CargoType ct = inputCargoType();

    // 输入重量
    double w;
    cout << "货物重量(kg): ";
    w = readDouble();
    if (w <= 0) {
        cout << "重量必须大于0！下单失败。" << endl;
        waitForReturn();
        return;
    }

    // 检查是否有合适的无人机能配送（提示用户但不阻止下单）
    bool canDeliver = false;
    for (size_t i = 0; i < lightDrones.size(); i++) {
        if (lightDrones[i].canCarry(ct) && w <= lightDrones[i].getMaxWeight()) { canDeliver = true; break; }
    }
    if (!canDeliver) {
        for (size_t i = 0; i < mediumDrones.size(); i++) {
            if (mediumDrones[i].canCarry(ct) && w <= mediumDrones[i].getMaxWeight()) { canDeliver = true; break; }
        }
    }
    if (!canDeliver) {
        for (size_t i = 0; i < heavyDrones.size(); i++) {
            if (heavyDrones[i].canCarry(ct) && w <= heavyDrones[i].getMaxWeight()) { canDeliver = true; break; }
        }
    }
    if (!canDeliver) {
        cout << "⚠ 警告：当前没有无人机能配送此订单（可能超重或无匹配等级的无人机）！" << endl;
        cout << "是否继续下单？(y/n): ";
        char confirm;
        cin >> confirm;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        if (confirm != 'y' && confirm != 'Y') {
            cout << "已取消下单。" << endl;
            waitForReturn();
            return;
        }
    }

    // 选择配送方式：即日配送 或 预约配送
    cout << "\n请选择配送方式：" << endl;
    cout << "  1. 即日配送" << endl;
    cout << "  2. 预约配送（可提前1-3天）" << endl;
    cout << "请选择 (1/2): ";
    int modeChoice;
    modeChoice = readInt();

    time_t schedTime = 0;    // 预约时间戳，0=即日
    string timeSlot = "";    // 时段描述

    if (modeChoice == 2) {
        // 预约配送：选择1-3天后的日期
        time_t now = time(0);
        cout << "\n可选预约日期（最多提前3天）：" << endl;
        for (int day = 1; day <= 3; day++) {
            time_t dayTime = now + day * 24 * 60 * 60;   // 计算第day天
            tm* dtm = localtime(&dayTime);
            cout << "  " << day << ". "
                 << (1900 + dtm->tm_year) << "-"
                 << setw(2) << setfill('0') << (1 + dtm->tm_mon) << "-"
                 << setw(2) << setfill('0') << dtm->tm_mday << endl;
        }
        cout << "请选择预约天数 (1-3): ";
        int dayChoice;
        dayChoice = readInt();
        if (dayChoice < 1 || dayChoice > 3) {
            cout << "无效选择，默认即日配送。" << endl;
        } else {
            schedTime = now + dayChoice * 24 * 60 * 60;  // 计算预约日期时间戳

            // 选择配送时段
            cout << "\n请选择配送时段：" << endl;
            cout << "  1. 上午（8:00-12:00）" << endl;
            cout << "  2. 下午（12:00-17:00）" << endl;
            cout << "  3. 晚上（17:00-21:00）" << endl;
            cout << "请选择 (1-3): ";
            int slotChoice;
            slotChoice = readInt();
            switch (slotChoice) {
                case 1: timeSlot = "上午8-12"; break;
                case 2: timeSlot = "下午12-17"; break;
                case 3: timeSlot = "晚上17-21"; break;
                default:
                    cout << "无效选择，默认上午时段。" << endl;
                    timeSlot = "上午8-12";
            }
        }
    }

    // 选择配送站点：显示4个站点供用户选择起点和终点
    cout << "\n【配送地图】A=天马公寓  B=德智公寓  C=综合楼  D=东方红广场" << endl;
    cout << "相邻站点间配送时长：" << SEGMENT_TIME << " 分钟" << endl;
    cout << "\n请选择配送起点：" << endl;
    for (int i = 0; i < 4; i++) {
        cout << "  " << (i + 1) << ". " << STATION_CODES[i]
             << " — " << STATION_NAMES[i] << endl;
    }
    cout << "请选择 (1-4): ";
    int fromChoice;
    fromChoice = readInt();
    if (fromChoice < 1 || fromChoice > 4) {
        cout << "无效选择，默认起点为天马公寓(A)。" << endl;
        fromChoice = 1;
    }
    string fromStation = STATION_CODES[fromChoice - 1];

    cout << "\n请选择配送终点：" << endl;
    for (int i = 0; i < 4; i++) {
        cout << "  " << (i + 1) << ". " << STATION_CODES[i]
             << " — " << STATION_NAMES[i] << endl;
    }
    cout << "请选择 (1-4): ";
    int toChoice;
    toChoice = readInt();
    if (toChoice < 1 || toChoice > 4) {
        cout << "无效选择，默认终点为天马公寓(A)。" << endl;
        toChoice = 1;
    }
    string toStation = STATION_CODES[toChoice - 1];
    int delMinutes = calcDeliveryTime(fromStation, toStation);

    while (fromStation == toStation) {
        cout << "\n起点和终点不能相同！请重新选择配送终点：" << endl;
        for (int i = 0; i < 4; i++) {
            if (STATION_CODES[i] != fromStation) {
                cout << "  " << (i + 1) << ". " << STATION_CODES[i]
                     << " — " << STATION_NAMES[i] << endl;
            }
        }
        cout << "请选择: ";
        toChoice = readInt();
        if (toChoice < 1 || toChoice > 4) {
            cout << "无效选择，默认终点为天马公寓(A)。" << endl;
            toChoice = 1;
        }
        toStation = STATION_CODES[toChoice - 1];
    }
    delMinutes = calcDeliveryTime(fromStation, toStation);
    cout << "\n预计配送时长：约 " << delMinutes << " 分钟（"
         << fromStation << " → " << toStation << "）" << endl;

    // 输入收件人信息
    string rname, rphone, raddr;
    cout << "\n收件人姓名: ";
    getline(cin, rname);
    cout << "收件人电话: ";
    getline(cin, rphone);
    cout << "详细地址: ";
    getline(cin, raddr);

    // 输入货物描述
    string desc;
    cout << "货物描述: ";
    getline(cin, desc);

    // 生成订单编号并创建订单
    string oid = generateOrderID();
    Order newOrder(oid, ct, rname, rphone, raddr, w, desc, Pending, userName, schedTime, timeSlot, fromStation, toStation, delMinutes);
    allOrders.push_back(newOrder);  // 添加到全局订单列表
    cout << "\n下单成功！" << endl;
    cout << "订单编号：" << oid << endl;
    cout << "货物类型：" << cargoTypeToString(ct) << endl;
    if (schedTime > 0) {
        tm* stm = localtime(&schedTime);
        cout << "预约配送日期："
             << (1900 + stm->tm_year) << "-"
             << setw(2) << setfill('0') << (1 + stm->tm_mon) << "-"
             << setw(2) << setfill('0') << stm->tm_mday
             << "  时段：" << timeSlot << endl;
    } else {
        cout << "配送方式：即日配送" << endl;
    }
    waitForReturn();
}

// =====================================================================
// 用户功能2：查看当前用户的所有订单
// =====================================================================
void userShowMyOrders(string userName) {
    clearScreen();
    cout << "\n=========== 我的订单 [" << userName << "] ===========" << endl;
    bool found = false;
    for (size_t i = 0; i < allOrders.size(); i++) {
        if (allOrders[i].getOwnerName() == userName) {
            cout << "\n--- 订单 #" << (i + 1) << " ---" << endl;
            allOrders[i].Show();
            found = true;
        }
    }
    if (!found) {
        cout << "您还没有任何订单！" << endl;
    }
    waitForReturn();
}

// =====================================================================
// 用户功能3：确认收货（将"已送达"订单状态改为"已收货"）
// =====================================================================
void userConfirmReceive(string userName) {
    clearScreen();
    cout << "\n=========== 确认收货 ===========" << endl;

    // 列出当前用户所有"已送达"状态的订单
    cout << "\n--- 可确认收货的订单 ---" << endl;
    int deliveredCount = 0;
    vector<int> deliveredIndices;  // 记录已送达订单的索引
    for (size_t i = 0; i < allOrders.size(); i++) {
        if (allOrders[i].getOwnerName() == userName
            && allOrders[i].getOrderState() == Delivered) {
            cout << deliveredCount + 1 << ". 订单 " << allOrders[i].getOrderID()
                 << " | " << cargoTypeToString(allOrders[i].getCargoType())
                 << " | " << allOrders[i].getDescription() << endl;
            deliveredIndices.push_back(i);
            deliveredCount++;
        }
    }

    if (deliveredCount == 0) {
        cout << "您没有待收货的订单！" << endl;
        waitForReturn();
        return;
    }

    // 选择要确认收货的订单
    cout << "请选择要确认收货的订单 (1-" << deliveredCount << "): ";
    int choice;
    choice = readInt();
    if (choice < 1 || choice > deliveredCount) {
        cout << "无效选择！" << endl;
        waitForReturn();
        return;
    }

    // 将订单状态改为"已收货"
    int orderIndex = deliveredIndices[choice - 1];
    allOrders[orderIndex].setOrderState(Received);
    cout << "\n订单 " << allOrders[orderIndex].getOrderID() << " 确认收货成功！感谢您的使用。" << endl;
    waitForReturn();
}

// =====================================================================
// 用户功能4：取消订单（仅待处理/已分配状态可取消，记录取消次数）
// =====================================================================
void userCancelOrder(string userName) {
    clearScreen();
    cout << "\n=========== 取消订单 ===========" << endl;

    // 先从 accounts 中找到当前用户
    Account* myAcc = NULL;
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].username == userName) {
            myAcc = &accounts[i];
            break;
        }
    }
    if (!myAcc) { cout << "错误：找不到账户！" << endl; waitForReturn(); return; }

    // 列出当前用户可取消的订单（仅待处理/已分配）
    cout << "\n--- 可取消的订单 ---" << endl;
    int cancelCount = 0;
    vector<int> cancelIndices;  // 记录可取消订单的索引
    for (size_t i = 0; i < allOrders.size(); i++) {
        if (allOrders[i].getOwnerName() == userName
            && (allOrders[i].getOrderState() == Pending
                || allOrders[i].getOrderState() == Assigned)) {
            cout << cancelCount + 1 << ". 订单 " << allOrders[i].getOrderID()
                 << " | " << cargoTypeToString(allOrders[i].getCargoType())
                 << " | " << allOrders[i].getDescription()
                 << " | 状态：" << orderStateToString(allOrders[i].getOrderState()) << endl;
            cancelIndices.push_back(i);
            cancelCount++;
        }
    }

    if (cancelCount == 0) {
        cout << "您没有可取消的订单！（配送中/已送达/已收货的订单不可取消）" << endl;
        waitForReturn();
        return;
    }

    // 选择要取消的订单
    cout << "请选择要取消的订单 (1-" << cancelCount << "): ";
    int choice;
    choice = readInt();
    if (choice < 1 || choice > cancelCount) {
        cout << "无效选择！" << endl;
        waitForReturn();
        return;
    }

    // 确认取消
    int orderIndex = cancelIndices[choice - 1];
    cout << "\n确认取消订单 " << allOrders[orderIndex].getOrderID() << "？(y/n): ";
    char confirm;
    cin >> confirm;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    if (confirm != 'y' && confirm != 'Y') {
        cout << "已取消操作。" << endl;
        waitForReturn();
        return;
    }

    // 如果是已分配状态，需要释放被分配的无人机
    if (allOrders[orderIndex].getOrderState() == Assigned) {
        string cancelledOrderID = allOrders[orderIndex].getOrderID();
        for (size_t j = 0; j < lightDrones.size(); j++) {
            if (lightDrones[j].getAssignedOrderID() == cancelledOrderID) {
                lightDrones[j].setAssignedOrderID("#");
                lightDrones[j].setAssignedBy("#");
            }
        }
        for (size_t j = 0; j < mediumDrones.size(); j++) {
            if (mediumDrones[j].getAssignedOrderID() == cancelledOrderID) {
                mediumDrones[j].setAssignedOrderID("#");
                mediumDrones[j].setAssignedBy("#");
            }
        }
        for (size_t j = 0; j < heavyDrones.size(); j++) {
            if (heavyDrones[j].getAssignedOrderID() == cancelledOrderID) {
                heavyDrones[j].setAssignedOrderID("#");
                heavyDrones[j].setAssignedBy("#");
            }
        }
    }
    allOrders[orderIndex].setOrderState(Cancelled); // 订单状态变为"已取消"
    cout << "\n订单 " << allOrders[orderIndex].getOrderID() << " 已取消！" << endl;
    recordCancellation(*myAcc);  // 记录取消次数（达到3次自动封禁）
    waitForReturn();
}

// =====================================================================
// 用户主循环：接收菜单选择并分发到对应功能
// =====================================================================
void userLoop(string userName) {
    int choice;
    do {
        clearScreen();
        displayUserMenu(userName);
        choice = readInt();
        switch (choice) {
            case 1: userPlaceOrder(userName);       break;  // 下单（即日/预约）
            case 2: userShowMyOrders(userName);     break;  // 查看我的订单
            case 3: userConfirmReceive(userName);   break;  // 确认收货
            case 4: userCancelOrder(userName);      break;  // 取消订单
            case 0: cout << "返回主菜单..." << endl; break;
            default: cout << "无效选项，请重新选择！" << endl; waitForReturn();
        }
    } while (choice != 0);
}

// =====================================================================
// 注册新用户：提供用户名、密码、确认密码、身份证号、电话号码
// =====================================================================
void userRegister() {
    clearScreen();
    cout << "\n=========== 用户注册 ===========" << endl;

    string username, password, confirmPwd, idCard, phone;

    // 输入用户名
    cout << "用户名: ";
    getline(cin, username);
    // 检查用户名是否已存在
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].username == username) {
            cout << "该用户名已被注册！注册失败。" << endl;
            waitForReturn();
            return;
        }
    }

    // 输入密码
    cout << "密码: ";
    getline(cin, password);
    // 确认密码
    cout << "确认密码: ";
    getline(cin, confirmPwd);
    if (password != confirmPwd) {
        cout << "两次密码不一致！注册失败。" << endl;
        waitForReturn();
        return;
    }

    // 输入身份证号
    cout << "身份证号: ";
    getline(cin, idCard);
    if (idCard.empty()) {
        cout << "身份证号不能为空！注册失败。" << endl;
        waitForReturn();
        return;
    }

    // 输入电话号码
    cout << "电话号码: ";
    getline(cin, phone);
    if (phone.empty()) {
        cout << "电话号码不能为空！注册失败。" << endl;
        waitForReturn();
        return;
    }

    // 创建新账户，角色为普通用户
    time_t now = time(0);
    tm* nowTm = localtime(&now);
    tm weekStart = *nowTm;
    weekStart.tm_hour = 0; weekStart.tm_min = 0; weekStart.tm_sec = 0;
    int daysFromMonday = nowTm->tm_wday == 0 ? 6 : nowTm->tm_wday - 1;
    weekStart.tm_mday -= daysFromMonday;
    time_t weekStartTime = mktime(&weekStart);

    Account newAcc;
    newAcc.username = username;
    newAcc.password = password;
    newAcc.role = "user";
    newAcc.displayName = username;  // 默认显示名为用户名
    newAcc.idCard = idCard;
    newAcc.phone = phone;
    newAcc.cancelCount = 0;
    newAcc.cancelWeekStart = weekStartTime;
    newAcc.bannedUntil = 0;
    accounts.push_back(newAcc);

    cout << "\n注册成功！请使用用户名 \"" << username << "\" 登录。" << endl;
    cout << "（身份证号将在下单时用于身份验证）" << endl;
    waitForReturn();
}

// =====================================================================
// 主菜单：选择角色进入对应子系统
// =====================================================================
void displayMainMenu() {
    cout << "\n=========== 无人机送货系统 ===========" << endl;
    cout << "请选择您的角色：" << endl;
    cout << "  1. 管理员" << endl;
    cout << "  2. 配送员" << endl;
    cout << "  3. 普通用户" << endl;
    cout << "  4. 注册新用户" << endl;
    cout << "  0. 退出系统" << endl;
    cout << "=======================================" << endl;
    cout << "请选择 (0-4): ";
}

// =====================================================================
// 主函数：系统入口，初始化数据后进入角色选择主循环
// =====================================================================
int main() {
    // 切换控制台编码为 UTF-8，防止中文乱码
    system("chcp 65001 > nul");
    // 初始化预置数据：无人机 + 示例订单 + 账户
    initPresetData();
    initAccounts();

    int choice;
    do {
        clearScreen();
        displayMainMenu();
        choice = readInt();

        switch (choice) {
            case 1: {
                // 管理员：输入账号密码登录
                clearScreen();
                cout << "\n=========== 管理员登录 ===========" << endl;
                Account* acc = login("admin");
                if (acc == NULL) {
                    cout << "\n登录失败！账号或密码错误。" << endl;
                    waitForReturn();
                    break;
                }
                cout << "\n管理员登录成功！欢迎回来。" << endl;
                waitForReturn();
                adminLoop();
                break;
            }
            case 2: {
                // 配送员：输入账号密码登录
                clearScreen();
                cout << "\n=========== 配送员登录 ===========" << endl;
                Account* acc = login("deliver");
                if (acc == NULL) {
                    cout << "\n登录失败！账号或密码错误。" << endl;
                    waitForReturn();
                    break;
                }
                cout << "\n" << acc->displayName << " 登录成功！" << endl;
                waitForReturn();
                deliverLoop(acc->username);  // 传入账号名作为配送员标识
                break;
            }
            case 3: {
                // 普通用户：输入账号密码登录
                clearScreen();
                cout << "\n=========== 用户登录 ===========" << endl;
                Account* acc = login("user");
                if (acc == NULL) {
                    cout << "\n登录失败！账号或密码错误。" << endl;
                    waitForReturn();
                    break;
                }
                // 封禁提醒：被封禁用户仍可登录，但不能下单
                resetCancelCountIfNewWeek(*acc);   // 先刷新跨周数据
                if (isBanned(*acc)) {
                    cout << "\n⚠ 您的账号已被封禁，无法下单！（至 "
                         << formatBanTime(acc->bannedUntil) << "，剩余 "
                         << getBanRemainingDays(*acc) << " 天）" << endl;
                }
                cout << "\n用户 " << acc->displayName << " 登录成功！" << endl;
                cout << "（本周已取消订单 " << acc->cancelCount << " / 3 次）" << endl;
                waitForReturn();
                userLoop(acc->username);  // 传入账号名作为用户标识
                break;
            }
            case 4: {
                // 注册新用户
                userRegister();
                break;
            }
            case 0: {
                // 退出系统
                cout << "\n感谢使用无人机送货系统，再见！" << endl;
                break;
            }
            default: {
                cout << "无效选项，请重新选择！" << endl;
                waitForReturn();
            }
        }
    } while (choice != 0);

    return 0;
}
