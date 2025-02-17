## B站同步视频讲解
- https://www.bilibili.com/video/BV1Xw411f7FW

## 3行代码实现极致性能YoloV5/YoloX推理，TensorRT C++库
1. 支持最新版tensorRT8.0，具有最新的解析器算子支持
2. 支持静态显性batch size，和动态非显性batch size，这是官方所不支持的
3. 支持自定义插件，简化插件的实现过程
4. 支持fp32、fp16、int8的编译
5. 优化代码结构，打印编译网络信息
6. 支持RetinaFace、Scrfd、YoloV5、YoloX、Arcface、AlphaPose、DeepSORT
7. c++类库，对编译和推理做了封装，对tensor做了封装，支持n维的tensor管理

## Windows支持
1. 依赖请查看[lean/README.md](lean/README.md)
2. TensorRT.vcxproj文件中，修改`<Import Project="$(VCTargetsPath)\BuildCustomizations\CUDA 10.0.props" />`为你配置的CUDA路径
3. TensorRT.vcxproj文件中，修改`<Import Project="$(VCTargetsPath)\BuildCustomizations\CUDA 10.0.targets" />`为你配置的CUDA路径
4. TensorRT.vcxproj文件中，修改`<CodeGeneration>compute_61,sm_61</CodeGeneration>`为你显卡配备的计算能力
    - 根据型号参考这里：https://developer.nvidia.com/zh-cn/cuda-gpus#compute
5. 配置依赖，或者下载依赖到lean中。配置VC++目录->包含目录和引用目录
6. 配置环境，调试->环境，设置PATH路径
7. 编译并运行案例

## Python支持
- 请在Makefile中设置`use_python := true`启用python支持，并编译生成trtpyc.so，使用`make trtpyc -j64`
- YoloV5的tensorRT推理
```python
yolo   = tp.Yolo(engine_file, type=tp.YoloType.X)
image  = cv2.imread("inference/car.jpg")
bboxes = yolo.commit(image).get()
```

- Pytorch无缝对接
```python
model     = models.resnet18(True).eval().to(device)
trt_model = tp.convert_torch_to_trt(model, input)
trt_out   = trt_model(input)
```

- 编译并安装:
    - 在CMakeLists.txt中修改`set(HAS_PYTHON ON)`
    - 执行编译`make pyinstall -j8`
    - 在使用时导入trtpy：`import trtpy as tp`

## Python接口导出Onnx和trtmodel
- 使用Python接口可以一句话导出Onnx和trtmodel，一次性调试发生的问题，解决问题。并储存onnx为后续部署使用
```python
import trtpy

model = models.resnet18(True).eval()
trtpy.from_torch(
    model, 
    dummy_input, 
    max_batch_size=16, 
    onnx_save_file="test.onnx", 
    engine_save_file="engine.trtmodel"
)
```

## 建议
- PyTorch >= 1.8，其他版本也可以用，遇到问题可以群里讨论
- TensorRT >= 8.0，目前只对8以及以上做了适配
- CUDA >= 10.2，因为TensorRT8最低要求10.2，再低不行了

## 3行代码实现YoloV5的高性能推理
```C++

// 创建推理引擎在0显卡上
//auto engine = Yolo::create_infer("yolox_m.fp32.trtmodel", Yolo::Type::X, 0);
auto engine = Yolo::create_infer("yolov5m.fp32.trtmodel", Yolo::Type::V5, 0);

// 加载图像
auto image = cv::imread("1.jpg");

// 推理并获取结果
auto box = engine->commit(image).get();
```

## 效果图
![](workspace/yq.jpg)

## YoloV5-ONNX推理支持-第一种，使用提供的onnx
- 这个yolov5m.onnx模型使用官方最新版本直接导出得到
- CMake
    - 在CMakeLists.txt中配置依赖路径tensorRT、cuda、cudnn、protobuf
    ```bash
    git clone git@github.com:shouxieai/tensorRT_cpp.git
    cd tensorRT_cpp

    mkdir build
    cd build
    cmake ..
    make yolo -j32

    # 或者make alphapose -j32
    ```

- Makefile
    - 在Makefile中配置好依赖的tensorRT、cuda、cudnn、protobuf
    ```bash
    git clone git@github.com:shouxieai/tensorRT_cpp.git
    cd tensorRT_cpp
    make yolo -j32
    ```

## YoloV5-ONNX推理支持-第二种，自行从官方导出onnx
- yolov5的onnx，你的pytorch版本>=1.7时，导出的onnx模型可以直接被当前框架所使用
- 你的pytorch版本低于1.7时，或者对于yolov5其他版本（2.0、3.0、4.0），可以对opset进行简单改动后直接被框架所支持
- 如果你想实现低版本pytorch的tensorRT推理、动态batchsize等更多更高级的问题，请打开我们[博客地址](http://zifuture.com:8090)后找到二维码进群交流
1. 下载yolov5
```bash
git clone git@github.com:ultralytics/yolov5.git
```

2. 修改代码，保证动态batchsize
```python
# yolov5/models/yolo.py第55行，forward函数 
# bs, _, ny, nx = x[i].shape  # x(bs,255,20,20) to x(bs,3,20,20,85)
# x[i] = x[i].view(bs, self.na, self.no, ny, nx).permute(0, 1, 3, 4, 2).contiguous()
# 修改为:

bs, _, ny, nx = x[i].shape  # x(bs,255,20,20) to x(bs,3,20,20,85)
bs = -1
ny = int(ny)
nx = int(nx)
x[i] = x[i].view(bs, self.na, self.no, ny, nx).permute(0, 1, 3, 4, 2).contiguous()

# yolov5/models/yolo.py第70行
#  z.append(y.view(bs, -1, self.no))
# 修改为：
z.append(y.view(bs, self.na * ny * nx, self.no))

# yolov5/export.py第52行
#torch.onnx.export(dynamic_axes={'images': {0: 'batch', 2: 'height', 3: 'width'},  # shape(1,3,640,640)
#                                'output': {0: 'batch', 1: 'anchors'}  # shape(1,25200,85)  修改为
torch.onnx.export(dynamic_axes={'images': {0: 'batch'},  # shape(1,3,640,640)
                                'output': {0: 'batch'}  # shape(1,25200,85) 
```

3. 导出onnx模型
```bash
cd yolov5
python export.py --weights=yolov5s.pt --dynamic --opset=11
```
4. 复制模型并执行
```bash
cp yolov5/yolov5m.onnx tensorRT_cpp/workspace/
cd tensorRT_cpp
make run -j32
```

## YoloX的支持
- https://github.com/Megvii-BaseDetection/YOLOX
- 你可以选择直接make run，会从镜像地址下载onnx并推理运行看到效果。不需要自行导出
1. 下载YoloX
```bash
git clone git@github.com:Megvii-BaseDetection/YOLOX.git
cd YOLOX
```

2. 修改代码
- 这是保证int8能够顺利编译和性能提升的关键，否则提示`Missing scale and zero-point for tensor (Unnamed Layer* 686)`
- 这是保证模型推理正常顺利的关键，虽然部分情况不修改也可以执行
```Python
# yolox/models/yolo_head.py的206行forward函数，替换为下面代码
# self.hw = [x.shape[-2:] for x in outputs]
self.hw = [list(map(int, x.shape[-2:])) for x in outputs]


# yolox/models/yolo_head.py的208行forward函数，替换为下面代码
# [batch, n_anchors_all, 85]
# outputs = torch.cat(
#     [x.flatten(start_dim=2) for x in outputs], dim=2
# ).permute(0, 2, 1)
proc_view = lambda x: x.view(-1, int(x.size(1)), int(x.size(2) * x.size(3)))
outputs = torch.cat(
    [proc_view(x) for x in outputs], dim=2
).permute(0, 2, 1)


# yolox/models/yolo_head.py的253行decode_outputs函数，替换为下面代码
#outputs[..., :2] = (outputs[..., :2] + grids) * strides
#outputs[..., 2:4] = torch.exp(outputs[..., 2:4]) * strides
#return outputs
xy = (outputs[..., :2] + grids) * strides
wh = torch.exp(outputs[..., 2:4]) * strides
return torch.cat((xy, wh, outputs[..., 4:]), dim=-1)


# tools/export_onnx.py的77行
model.head.decode_in_inference = True
```

3. 导出onnx模型
```bash
# 下载模型，或许你需要翻墙
# wget https://github.com/Megvii-BaseDetection/YOLOX/releases/download/0.1.1rc0/yolox_m.pth

# 导出模型
python tools/export_onnx.py -c yolox_m.pth -f exps/default/yolox_m.py --output-name=yolox_m.onnx --dynamic
```

4. 执行程序
```bash
cp YOLOX/yolox_m.onnx tensorRT_cpp/workspace/
cd tensorRT_cpp
make run -j32
```

## RetinaFace人脸检测支持
- https://github.com/biubug6/Pytorch_Retinaface
1. 下载Pytorch_Retinaface
```bash
git clone git@github.com:biubug6/Pytorch_Retinaface.git
cd Pytorch_Retinaface
```

2. 下载模型，请访问：https://github.com/biubug6/Pytorch_Retinaface#training 的training节点找到下载地址，解压到weights目录下，主要用到mobilenet0.25_Final.pth文件
3. 修改代码
```python
# models/retinaface.py第24行，
# return out.view(out.shape[0], -1, 2) 修改为
return out.view(-1, int(out.size(1) * out.size(2) * 2), 2)

# models/retinaface.py第35行，
# return out.view(out.shape[0], -1, 4) 修改为
return out.view(-1, int(out.size(1) * out.size(2) * 2), 4)

# models/retinaface.py第46行，
# return out.view(out.shape[0], -1, 10) 修改为
return out.view(-1, int(out.size(1) * out.size(2) * 2), 10)

# 以下是保证resize节点输出是按照scale而非shape，从而让动态大小和动态batch变为可能
# models/net.py第89行，
# up3 = F.interpolate(output3, size=[output2.size(2), output2.size(3)], mode="nearest") 修改为
up3 = F.interpolate(output3, scale_factor=2, mode="nearest")

# models/net.py第93行，
# up2 = F.interpolate(output2, size=[output1.size(2), output1.size(3)], mode="nearest") 修改为
up2 = F.interpolate(output2, scale_factor=2, mode="nearest")

# 以下代码是去掉softmax（某些时候有bug），同时合并输出为一个，简化解码部分代码
# models/retinaface.py第123行
# if self.phase == 'train':
#     output = (bbox_regressions, classifications, ldm_regressions)
# else:
#     output = (bbox_regressions, F.softmax(classifications, dim=-1), ldm_regressions)
# return output
# 修改为
output = (bbox_regressions, classifications, ldm_regressions)
return torch.cat(output, dim=-1)

# 添加opset_version=11，使得算子按照预期导出
# torch_out = torch.onnx._export(net, inputs, output_onnx, export_params=True, verbose=False,
#     input_names=input_names, output_names=output_names)
torch_out = torch.onnx._export(net, inputs, output_onnx, export_params=True, verbose=False, opset_version=11,
    input_names=input_names, output_names=output_names)
```
4. 执行导出onnx
```bash
python convert_to_onnx.py
```

5. 执行
```bash
cp FaceDetector.onnx ../tensorRT_cpp/workspace/mb_retinaface.onnx
cd ../tensorRT_cpp
make retinaface -j64
```

## Scrfd支持
- https://github.com/deepinsight/insightface/tree/master/detection/scrfd
- 具体导出Onnx的注意事项和方法，请加群沟通。等待后面更新

## ArcFace人脸识别支持
- https://github.com/deepinsight/insightface/tree/master/recognition/arcface_torch
```C++
auto arcface = Arcface::create_infer("arcface_iresnet50.fp32.trtmodel", 0);
auto feature = arcface->commit(make_tuple(face, landmarks)).get();
cout << feature << endl;  // 1x512
```
- 人脸识别案例中，`workspace/face/library`目录为注册入库人脸
- 人脸识别案例中，`workspace/face/recognize`目录为待识别的照片
- 结果储存在`workspace/face/result`和`workspace/face/library_draw`中

## 推理
```C++

// 创建推理引擎在0显卡上
auto engine = Yolo::create_infer("yolox_m.fp32.trtmodel"， Yolo::Type::X, 0);

// 加载图像
auto image = cv::imread("1.jpg");

// 推理并获取结果
auto box = engine->commit(image).get();
```

## 项目依赖的配置
- 考虑方便，这里有打包好的依赖项
    - 下载地址：[lean-tensorRT8.0.1.6-protobuf3.11.4-cudnn8.2.2.tar.gz](http://zifuture.com:1556/fs/25.shared/lean-tensorRT8.0.1.6-protobuf3.11.4-cudnn8.2.2.tar.gz)
1. 推荐使用Linux、VSCode，当然也可以支持windows
2. 在Makefile中配置你的cudnn、cuda、tensorRT8.0、protobuf路径
3. 在.vscode/c_cpp_properties.json中配置你的库路径
3. CUDA版本：CUDA10.2
4. CUDNN版本：cudnn8.2.2.26，注意下载dev（h文件）和runtime（so文件）
5. tensorRT版本：tensorRT-8.0.1.6-cuda10.2
6. protobuf版本（用于onnx解析器）：这里使用的是protobufv3.11.4
    - 下载地址：https://github.com/protocolbuffers/protobuf/tree/v3.11.4


## 模型编译-FP32/16
```cpp
TRT::compile(
  TRT::Mode::FP32,   // 使用fp32模型编译
  3,                          // max batch size
  "plugin.onnx",              // onnx 文件
  "plugin.fp32.trtmodel",     // 保存的文件路径
  {}                         // 重新定制输入的shape
);
```
- 对于FP32编译，只需要提供onnx文件即可，可以允许重定义onnx输入节点的shape
- 对于动态或者静态batch的支持，仅仅只需要一个选项，这对于官方发布的解析器是不支持的

## 模型编译-INT8
- 众所周知，int8的推理效果比fp32稍微差一点（预计-5%的损失），但是速度确快很多很多，这里通过集成的编译方式，很容易实现int8的编译工作
```cpp
// 定义int8的标定数据处理函数，读取数据并交给tensor的函数
auto int8process = [](int current, int count, vector<string>& images, shared_ptr<TRT::Tensor>& tensor){
    for(int i = 0; i < images.size(); ++i){

	// 对于int8的编译需要进行标定，这里读取图像数据并通过set_norm_mat到tensor中
        auto image = cv::imread(images[i]);
        cv::resize(image, image, cv::Size(640, 640));
        float mean[] = {0, 0, 0};
        float std[]  = {1, 1, 1};
        tensor->set_norm_mat(i, image, mean, std);
    }
};

// 编译模型指定为INT8
auto model_file = "yolov5m.int8.trtmodel";
TRT::compile(
  TRT::Mode::INT8,            // 选择INT8
  3,                          // max batch size
  "yolov5m.onnx",             // onnx文件
  model_file,                 // 编译后保存的文件
  {},                         // 重定义输入的shape
  int8process,                // 指定int8标定数据的处理回调函数
  ".",                        // 指定int8标定图像数据的目录
  ""                          // 指定int8标定后的数据储存/读取路径
);
```
- 避免了官方标定流程分离的问题，复杂度太高，在这里直接集成为一个函数处理

## 模型推理 
- 对于模型推理，封装了Tensor类，实现推理的维护和数据交互，对于数据从GPU到CPU过程完全隐藏细节
- 封装了Engine类，实现模型推理和管理
```cpp
// 模型加载，得到一个共享指针，如果为空表示加载失败
auto engine = TRT::load_infer("yolov5m.fp32.trtmodel");

// 打印模型信息
engine->print();

// 加载图像
auto image = imread("demo.jpg");

// 获取模型的输入和输出tensor节点，可以根据名字或者索引获取第几个
auto input = engine->input(0);
auto output = engine->output(0);

// 把图像塞到input tensor中，这里是减去均值，除以标准差
float mean[] = {0, 0, 0};
float std[]  = {1, 1, 1};
input->set_norm_mat(i, image, mean, std);

// 执行模型的推理，这里可以允许异步或者同步
engine->forward();

// 这里拿到的指针即是最终的结果指针，可以进行访问操作
float* output_ptr = output->cpu<float>();
// 这里对output_ptr进行处理即可得到结果
```

## 关于3080或者其他显卡
- 请调用tensorRT/common/cuda_tools.hpp中的device_capability函数，查询这个显卡的计算能力，然后配置Makefile或者CMakeLists中的计算能力为对应即可
- 例如`-gencode=arch=compute_75,code=sm_75`，例如3080Ti是86，则是：`-gencode=arch=compute_86,code=sm_86`
- 否则你可能能正常编译，但是结果却是随机的，错误的。或者直接报错
    - 根据型号参考这里：https://developer.nvidia.com/zh-cn/cuda-gpus#compute

## 一个插件的例子
- 只需要定义必要的核函数和推理过程，完全隐藏细节，隐藏插件的序列化、反序列化、注入
- 可以简洁的实现FP32、FP16两种格式支持的插件。具体参见代码HSwish cu/hpp
```cpp
template<>
__global__ void HSwishKernel(float* input, float* output, int edge) {

    KernelPositionBlock;
    float x = input[position];
    float a = x + 3;
    a = a < 0 ? 0 : (a >= 6 ? 6 : a);
    output[position] = x * a / 6;
}

int HSwish::enqueue(const std::vector<GTensor>& inputs, std::vector<GTensor>& outputs, const std::vector<GTensor>& weights, void* workspace, cudaStream_t stream) {

    int count = inputs[0].count();
    auto grid = CUDATools::grid_dims(count);
    auto block = CUDATools::block_dims(count);
    HSwishKernel <<<grid, block, 0, stream >>> (inputs[0].ptr<float>(), outputs[0].ptr<float>(), count);
    return 0;
}

RegisterPlugin(HSwish);
```

## 执行方式
1. 配置好Makefile中的依赖项路径
2. `make run -j64`即可

## 执行结果
```bash
[2021-07-22 14:37:11][info][_main.cpp:160]:===================== test fp32 ==================================
[2021-07-22 14:37:11][info][trt_builder.cpp:430]:Compile FP32 Onnx Model 'yolov5m.onnx'.
[2021-07-22 14:37:18][warn][trt_infer.cpp:27]:NVInfer WARNING: src/tensorRT/onnx_parser/ModelImporter.cpp:257: Change input batch size: images, final dimensions: (1, 3, 640, 640), origin dimensions: (5, 3, 640, 640)
[2021-07-22 14:37:18][info][trt_builder.cpp:548]:Input shape is 1 x 3 x 640 x 640
[2021-07-22 14:37:18][info][trt_builder.cpp:549]:Set max batch size = 3
[2021-07-22 14:37:18][info][trt_builder.cpp:550]:Set max workspace size = 1024.00 MB
[2021-07-22 14:37:18][info][trt_builder.cpp:551]:Dynamic batch dimension is true
[2021-07-22 14:37:18][info][trt_builder.cpp:554]:Network has 1 inputs:
[2021-07-22 14:37:18][info][trt_builder.cpp:560]:      0.[images] shape is 1 x 3 x 640 x 640
[2021-07-22 14:37:18][info][trt_builder.cpp:566]:Network has 3 outputs:
[2021-07-22 14:37:18][info][trt_builder.cpp:571]:      0.[470] shape is 1 x 255 x 80 x 80
[2021-07-22 14:37:18][info][trt_builder.cpp:571]:      1.[471] shape is 1 x 255 x 40 x 40
[2021-07-22 14:37:18][info][trt_builder.cpp:571]:      2.[472] shape is 1 x 255 x 20 x 20
[2021-07-22 14:37:18][verbo][trt_builder.cpp:575]:Network has 226 layers:
[2021-07-22 14:37:18][verbo][trt_builder.cpp:606]:  >>> 0.  Slice              1 x 3 x 640 x 640 -> 1 x 3 x 320 x 640 
[2021-07-22 14:37:18][verbo][trt_builder.cpp:606]:      1.  Slice              1 x 3 x 320 x 640 -> 1 x 3 x 320 x 320 
[2021-07-22 14:37:18][verbo][trt_builder.cpp:606]:  >>> 2.  Slice              1 x 3 x 640 x 640 -> 1 x 3 x 320 x 640 
[2021-07-22 14:37:18][verbo][trt_builder.cpp:606]:      3.  Slice              1 x 3 x 320 x 640 -> 1 x 3 x 320 x 320 
[2021-07-22 14:37:18][verbo][trt_builder.cpp:606]:  >>> 4.  Slice              1 x 3 x 640 x 640 -> 1 x 3 x 320 x 640 
[2021-07-22 14:37:18][verbo][trt_builder.cpp:606]:      5.  Slice              1 x 3 x 320 x 640 -> 1 x 3 x 320 x 320 
[2021-07-22 14:37:18][verbo][trt_builder.cpp:606]:  >>> 6.  Slice              1 x 3 x 640 x 640 -> 1 x 3 x 320 x 640 
[2021-07-22 14:37:18][verbo][trt_builder.cpp:606]:      7.  Slice              1 x 3 x 320 x 640 -> 1 x 3 x 320 x 320
[2021-07-22 14:37:18][verbo][trt_builder.cpp:606]:      222.LeakyRelu          1 x 768 x 20 x 20 -> 1 x 768 x 20 x 20 
[2021-07-22 14:37:18][verbo][trt_builder.cpp:606]:  *** 223.Convolution        1 x 192 x 80 x 80 -> 1 x 255 x 80 x 80 channel: 255, kernel: 1 x 1, padding: 0 x 0, stride: 1 x 1, dilation: 1 x 1, group: 1
[2021-07-22 14:37:18][verbo][trt_builder.cpp:606]:  *** 224.Convolution        1 x 384 x 40 x 40 -> 1 x 255 x 40 x 40 channel: 255, kernel: 1 x 1, padding: 0 x 0, stride: 1 x 1, dilation: 1 x 1, group: 1
[2021-07-22 14:37:18][verbo][trt_builder.cpp:606]:  *** 225.Convolution        1 x 768 x 20 x 20 -> 1 x 255 x 20 x 20 channel: 255, kernel: 1 x 1, padding: 0 x 0, stride: 1 x 1, dilation: 1 x 1, group: 1
[2021-07-22 14:37:18][info][trt_builder.cpp:615]:Building engine...
[2021-07-22 14:37:19][warn][trt_infer.cpp:27]:NVInfer WARNING: Detected invalid timing cache, setup a local cache instead
[2021-07-22 14:37:40][info][trt_builder.cpp:635]:Build done 22344 ms !
Engine 0x23dd7780 detail
        Max Batch Size: 3
        Dynamic Batch Dimension: true
        Inputs: 1
                0.images : shape {1 x 3 x 640 x 640}
        Outputs: 3
                0.470 : shape {1 x 255 x 80 x 80}
                1.471 : shape {1 x 255 x 40 x 40}
                2.472 : shape {1 x 255 x 20 x 20}
[2021-07-22 14:37:42][info][_main.cpp:77]:input.shape = 3 x 3 x 640 x 640
[2021-07-22 14:37:42][info][_main.cpp:96]:input->shape_string() = 3 x 3 x 640 x 640
[2021-07-22 14:37:42][info][_main.cpp:124]:outputs[0].size = 2
[2021-07-22 14:37:42][info][_main.cpp:124]:outputs[1].size = 5
[2021-07-22 14:37:42][info][_main.cpp:124]:outputs[2].size = 1
```


## 关于
- 我们的博客地址：http://www.zifuture.com:8090/
- 我们的B站地址：https://space.bilibili.com/1413433465