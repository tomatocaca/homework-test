# Qt Widgets 图形化迁移方案

## 一、整体思路：UI 与逻辑分离

现有代码所有业务逻辑和 UI（cin/cout）混在一个文件中。迁移的核心原则是：

```
现有架构：cin/cout 直接嵌入业务函数
目标架构：业务逻辑类 + Qt 界面类，通过信号槽连接
```

**可以原封不动保留的部分：**
- `CargoType` / `OrderState` / `DroneState` / `DroneLevel` 枚举
- `STATION_NAMES` / `STATION_CODES` / `SEGMENT_TIME` 等常量
- `calcDeliveryTime()` / `getStationName()` 等纯计算函数
- `Order` 类的成员变量、getter/setter、构造函数
- `Drone` 基类及其三个子类（`LightDrone` / `MediumDrone` / `HeavyDrone`）的属性和核心逻辑
- `Account` 结构体及封禁相关函数
- 全局数据容器 `allOrders`、`lightDrones`、`mediumDrones`、`heavyDrones`、`accounts`

**必须重写/删除的部分：**
- `clearScreen()` —— Qt 用布局管理，不需要清屏
- `waitForReturn()` —— Qt 用信号槽，不需要阻塞等待按键
- 所有 `cout` 输出 —— 改为 QLabel / QTextEdit / QTableWidget 显示
- 所有 `cin` 输入 —— 改为 QLineEdit / QComboBox / QSpinBox 获取
- 菜单导航（switch-case 循环）—— 改为 QStackedWidget 页面切换
- `main()` 函数 —— 改为 QApplication 启动

---

## 二、推荐文件结构

```
drone/
├── drone_delivery_system.cpp    (保留，但不再修改)
├── CMakeLists.txt               (或 .pro 文件，Qt 构建配置)
├── main.cpp                     (Qt 入口：QApplication + 主窗口)
├── model/
│   ├── order.h                  (Order 类声明)
│   ├── order.cpp                (Order 类实现)
│   ├── drone.h                  (Drone 基类 + 子类声明)
│   ├── drone.cpp                (Drone 基类 + 子类实现)
│   ├── account.h                (Account 结构体 + 封禁函数)
│   ├── account.cpp              (账户相关函数实现)
│   └── datacenter.h/cpp         (全局数据容器 + 初始化，替代全局变量)
├── widget/
│   ├── mainwindow.h/cpp         (主窗口：角色选择 + 页面切换)
│   ├── loginwidget.h/cpp        (登录界面)
│   ├── adminwidget.h/cpp        (管理员界面：6个功能子页面)
│   ├── deliverwidget.h/cpp      (配送员界面：4个功能子页面)
│   └── userwidget.h/cpp         (用户界面：4个功能子页面)
└── CMakeLists.txt               (Qt 项目构建文件)
```

---

## 三、具体改动对照表

### 3.1 入口函数 main()

| 现有代码 | Qt 改造 |
|---------|---------|
| `int main()` 循环选择角色 | `QApplication app(argc, argv);` 启动 |
| `system("chcp 65001 > nul")` | Qt 默认支持 UTF-8，删除 |
| `clearScreen()` | 删除，Qt 自动管理布局 |
| `cin >> choice` 菜单选择 | QPushButton 点击信号 |
| `switch-case` 角色切换 | QStackedWidget 切换页面 |

新 main.cpp 示例：
```cpp
#include <QApplication>
#include "widget/mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setFont(QFont("Microsoft YaHei", 10));  // 中文字体
    MainWindow w;
    w.show();
    return app.exec();
}
```

### 3.2 登录界面 loginwidget.h/cpp

| 现有代码 | Qt 控件 |
|---------|--------|
| `cout << "请输入账号: "` | QLabel + QLineEdit |
| `cin >> username` | QLineEdit::text() |
| `cin >> password` | QLineEdit（echoMode=Password） |
| `login()` 函数的返回值判断 | 信号槽：emit loginSuccess(role, account) |

```cpp
class LoginWidget : public QWidget {
    Q_OBJECT
public:
    explicit LoginWidget(QWidget *parent = nullptr);
signals:
    void loginSuccess(const QString& role, Account* account);
private:
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QComboBox *roleCombo;       // 选择角色：管理员/配送员/用户
    QPushButton *loginBtn;
};
```

### 3.3 管理员界面 adminwidget.h/cpp

将 `adminLoop()` 中的 6 个功能改为 6 个子页面，用 QStackedWidget 切换：

| 现有函数 | Qt 界面改造 |
|---------|-----------|
| `adminAddDrone()` | QComboBox 选等级 + QLineEdit 输入编号 + QPushButton 确认 |
| `adminShowAllDrones()` | QTableWidget 展示，每行一架无人机 |
| `adminShowAllOrders()` | QTableWidget 展示所有订单 |
| `adminShowReport()` | QLabel 显示统计数据，或用 QChart 画图表 |
| `adminShowDailyDeliveries()` | QTableWidget 展示配送记录 |
| `adminManageUsers()` | QTableWidget 列出用户 + QPushButton 解封 |

### 3.4 配送员界面 deliverwidget.h/cpp

| 现有函数 | Qt 界面改造 |
|---------|-----------|
| `deliverShowPendingOrders()` | QTableWidget 列出待处理订单 |
| `deliverAssignOrder()` | 两级选择：先 QComboBox 选订单，再 QComboBox 选无人机 |
| `deliverExecute()` | QComboBox 选任务 + QPushButton 执行，结果用 QMessageBox 提示 |
| `deliverShowRecords()` | QTableWidget 列出已处理订单 |

### 3.5 用户界面 userwidget.h/cpp

| 现有函数 | Qt 界面改造 |
|---------|-----------|
| `userPlaceOrder()` | 多步表单：QComboBox 选货物类型、QDoubleSpinBox 输入重量、QComboBox 选配送方式/日期/时段/起终点、QLineEdit 输入收件人信息 |
| `userShowMyOrders()` | QTableWidget 列出当前用户的订单 |
| `userConfirmReceive()` | QComboBox 选订单 + QPushButton 确认 |
| `userCancelOrder()` | QComboBox 选订单 + QMessageBox 确认 + 封禁提示 |

### 3.6 Order::Show() 方法

| 现有代码 | Qt 改造 |
|---------|--------|
| 多行 `cout` 输出 | 改为返回 `QString`，或提供一个静态方法将 Order 转为表格行数据 |

```cpp
// 删除 Show() 中的 cout，改为：
QString toDisplayString() const {
    return QString("订单编号：%1\n货物类型：%2\n重量：%3kg\n...")
        .arg(orderID)
        .arg(cargoTypeToString(cargoType))
        .arg(weight);
}
```

### 3.7 Drone::deliver() 方法

| 现有代码 | Qt 改造 |
|---------|--------|
| 配送过程中的 `cout` 输出 | 改为返回 `QString` 描述，由界面层显示 |
| 模拟配送"瞬间完成" | 可选：加 QTimer 模拟配送动画（先保留原有逻辑） |

```cpp
// deliver() 不再直接 cout，改为返回配送结果描述：
struct DeliveryResult {
    bool success;
    QString message;
};
virtual DeliveryResult deliver(vector<Order>& allOrders);
```

---

## 四、Visual Studio 原生构建配置

### 4.1 前置准备

1. 安装 **Qt**（建议 Qt 5.15 或 Qt 6.x，勾选 MSVC 64-bit 组件）
2. 安装 **Visual Studio**（2019 或 2022，勾选"使用 C++ 的桌面开发"工作负载）
3. 安装 VS 扩展 **Qt Visual Studio Tools**（VS 菜单 → 扩展 → 管理扩展 → 搜索 "Qt"）
4. 在 Qt VS Tools 中配置 Qt 版本路径（Qt VS Tools → Qt Options → 添加 Qt 安装目录）

### 4.2 创建项目

1. VS → 创建新项目 → **Qt Widgets Application**
2. 项目名称：`DroneDeliverySystem`
3. 在 Qt 项目向导中选择对应的 Qt 版本（MSVC 64-bit）
4. VS 会自动生成 `main.cpp`、`DroneDeliverySystem.ui`、`DroneDeliverySystem.h/cpp`

### 4.3 MOC 配置（Qt 元对象编译器）

Qt 的信号槽机制需要 MOC 处理。在 VS 中由 Qt VS Tools 自动处理，但需确保：

- 所有含 `Q_OBJECT` 宏的头文件，在属性页中 **Item Type** 设为 `Qt MOC Input`
- `.cpp` 文件中包含 `#include "xxx.moc"` 或头文件被正确 MOC（Qt VS Tools 通常自动识别）

### 4.4 项目属性配置

```
项目属性 → C/C++ → 语言 → C++语言标准 → ISO C++17 标准 (/std:c++17)
项目属性 → C/C++ → 常规 → 附加包含目录 → 添加 Qt 的 include 路径
    例如：C:\Qt\6.x.x\msvc_64\include
项目属性 → 链接器 → 常规 → 附加库目录 → 添加 Qt 的 lib 路径
    例如：C:\Qt\6.x.x\msvc_64\lib
项目属性 → 链接器 → 输入 → 附加依赖项 → 添加
    Qt6Widgets.lib（或 Qt5Widgets.lib）
```

> 使用 Qt VS Tools 创建的项目通常已自动配置好以上路径，无需手动设置。

---

## 五、迁移步骤（建议顺序）

1. **用 Qt VS Tools 创建 Qt Widgets 项目**，确保空白窗口能正常编译运行
2. **拆分 model 层**：将 Order、Drone、Account 类分别放入 model/ 目录下的 .h/.cpp 文件
3. **创建 mainwindow.h/cpp**，实现主窗口框架（QStackedWidget 角色选择页面）
4. **创建 loginwidget.h/cpp**，实现登录界面，连接信号槽跳转到对应角色页面
5. **逐个实现 adminwidget / deliverwidget / userwidget**，每个功能独立一个子页面
6. **美化界面**：添加样式表、图标、布局优化

---

## 六、注意事项

- 现有代码使用 `system("cls")` 清屏，Qt 中完全不需要，删除即可
- 现有代码大量使用 `cin.ignore()` 清缓冲区，Qt 中不存在此问题，全部删除
- 现有代码的 `time_t` 时间处理可以直接在 Qt 中使用，也可改用 `QDateTime`
- 中文显示：确保源文件保存为 UTF-8 编码 + BOM，或在 CMakeLists.txt 中设置编译选项
