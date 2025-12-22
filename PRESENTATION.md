# Reflective Shadow Maps (RSM) 实现报告

## 项目概述

本项目实现了 **Reflective Shadow Maps (RSM)** 算法，这是一种高效的实时间接光照技术，基于 Dachsbacher & Stamminger 在 2005 年发表的论文。

---

## 什么是 RSM？

### 传统 Shadow Map
- 仅存储深度信息
- 只能判断是否被遮挡
- 只能渲染直接光照 + 阴影

### Reflective Shadow Map
- 扩展 Shadow Map，存储更丰富的信息：
  - **Flux (通量)**：表面反射的光能量
  - **Normal (法线)**：表面朝向
  - **Position (位置)**：世界/视空间坐标
- 每个像素作为一个 **虚拟点光源 (VPL)**
- 实现 **一次反弹的间接光照**

---

## 核心算法原理

### 间接光照公式

$$E_p(x, n) = \Phi_p \cdot \frac{\max(0, \langle n_p | x - x_p \rangle) \cdot \max(0, \langle n | x_p - x \rangle)}{||x - x_p||^4}$$

其中：
- $\Phi_p$ = 像素光源的反射通量
- $n_p$ = VPL 法线
- $n$ = 接收点法线
- $x_p$ = VPL 位置
- $x$ = 接收点位置

### 采样策略
- 以当前点投影到 RSM 的坐标为中心
- 使用极坐标分布采样
- 密度随距离递减，权重随距离增加

---

## 实现架构

### 渲染管线 (4个Pass)

```
1. G-Buffer Pass
   └─ 渲染场景到 G-Buffer (Albedo, Normal, Position)

2. RSM Buffer Pass  
   └─ 从光源视角渲染 (Flux, Normal, Position)

3. Shading Pass
   └─ 采样 RSM，计算间接光照
   
4. Display Pass
   └─ 显示最终结果
```

### 技术选型
- **语言**: C++ / GLSL
- **图形API**: OpenGL 4.1 (macOS 兼容)
- **UI**: ImGUI
- **场景**: Sponza (经典室内场景)

---

## 功能特性

### 已实现
✅ 完整的 RSM 间接光照计算  
✅ 可调节 VPL 采样数量 (8-128)  
✅ 可调节采样半径和间接光强度  
✅ RSM 开关（对比直接光 vs 间接光效果）  
✅ 多种调试视图模式  
✅ 独立的光源和相机控制  

### 调试视图
- Final (最终结果)
- Albedo / Normal / Position
- RSM Flux / RSM Normal / RSM Position

---

## 效果对比

| 模式 | 说明 |
|------|------|
| Shadow Mapping | 仅直接光照 + 阴影，暗部全黑 |
| RSM Enabled | 直接光照 + 间接光照，暗部有柔和光照 |

RSM 带来的改进：
- 阴影区域不再全黑
- 墙面之间有颜色渗透
- 场景整体更加真实自然

---

## 性能考量

### 优化措施
1. **降低 RSM 分辨率** (256×256)
2. **限制 VPL 数量** (默认64个)
3. **使用片段着色器** (兼容性考虑)

### 可能的改进
- Screen-space 插值加速
- GPU Compute Shader (非 macOS)
- 层次化采样

---

## 代码结构

```
src/impl/
├── rsm_main.cpp      # 主程序
└── obj_loader.cpp    # OBJ 模型加载器

data/shaders/rsm/
├── gbuffer.vert/frag     # G-Buffer 渲染
├── rsm_buffer.vert/frag  # RSM 生成
├── rsm_shading.vert/frag # 间接光照计算
└── display.vert/frag     # 最终显示
```

---

## 运行演示

```bash
# 编译
cd build && cmake .. && make rsm

# 运行
./src/impl/rsm
```

### 交互操作
- **Light**: 调节光源方向和颜色
- **Camera**: 调节相机视角
- **Rendering Mode**: 开启/关闭 RSM
- **RSM Parameters**: 调节采样参数
- **Debug**: 查看各个 Buffer

---

## 总结

RSM 是一种在**实时渲染**和**全局光照**之间取得平衡的技术：
- 不需要预计算
- 支持动态场景
- 效果近似但足够 **plausible (可信)**
- 是后续更先进技术 (如 LPV, VXGI) 的基础

---

## 参考文献

Dachsbacher, C., & Stamminger, M. (2005). *Reflective Shadow Maps*. 
Proceedings of the 2005 Symposium on Interactive 3D Graphics and Games.

---

## Q&A

谢谢！欢迎提问。
