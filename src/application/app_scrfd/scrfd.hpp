#ifndef SCRFD_HPP
#define SCRFD_HPP

#include <vector>
#include <memory>
#include <string>
#include <future>
#include <opencv2/opencv.hpp>
#include "../common/face_detector.hpp"

namespace Scrfd{

    using namespace std;
    using namespace FaceDetector;

    class Infer{
    public:
        virtual shared_future<FaceBoxArray> commit(const cv::Mat& image) = 0;
        virtual vector<shared_future<FaceBoxArray>> commits(const vector<cv::Mat>& images) = 0;
        virtual tuple<cv::Mat, FaceBox> crop_face_and_landmark(
            const cv::Mat& image, const FaceBox& box, float scale_box=1.5f
        ) = 0;
    };

    shared_ptr<Infer> create_infer(const string& engine_file, int gpuid, float confidence_threshold=0.5f, float nms_threshold=0.5f);

}; // namespace Scrfd

#endif // SCRFD_HPP