#ifndef HSwish_HPP
#define HSwish_HPP

#include <onnxplugin/onnxplugin.hpp>

using namespace ONNXPlugin;

class HSwishConfig : public LayerConfig{
public:
	virtual void init() override;
};

class HSwish : public TRTPlugin {
public:
	SetupPlugin(HSwish);

	virtual std::shared_ptr<LayerConfig> config(const std::string& layerName) override;

	virtual nvinfer1::DimsExprs getOutputDimensions(
        	int32_t outputIndex, const nvinfer1::DimsExprs* inputs, int32_t nbInputs, nvinfer1::IExprBuilder& exprBuilder) noexcept override;

	int enqueue(const std::vector<GTensor>& inputs, std::vector<GTensor>& outputs, const std::vector<GTensor>& weights, void* workspace, cudaStream_t stream) override;
};

#endif //HSwish_HPP