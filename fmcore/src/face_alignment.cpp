#include "face_alignment.h"

#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

cv::Mat similarity_transform(const std::vector<cv::Point2f>& src, const std::vector<cv::Point2f>& dst) {
    CV_Assert(src.size() == dst.size() && src.size() >= 2);

    int num_points = static_cast<int>(src.size());

    // centroid calculation
    cv::Point2f src_center(0.f, 0.f), dst_center(0.f, 0.f);
    for (int i = 0; i < num_points; ++i) {
        src_center += src[i];
        dst_center += dst[i];
    }
    src_center *= (1.0f / num_points);
    dst_center *= (1.0f / num_points);

    // Centerize
    std::vector<cv::Point2f> src_demean(num_points), dst_demean(num_points);
    for (int i = 0; i < num_points; ++i) {
        src_demean[i] = src[i] - src_center;
        dst_demean[i] = dst[i] - dst_center;
    }

    // build Matrix H
    cv::Mat H = cv::Mat::zeros(2, 2, CV_32F);
    for (int i = 0; i < num_points; ++i) {
        H += (cv::Mat_<float>(2, 1) << src_demean[i].x, src_demean[i].y) *
             (cv::Mat_<float>(1, 2) << dst_demean[i].x, dst_demean[i].y);
    }
    H /= num_points;

    cv::Mat w, u, vt;
    cv::SVD::compute(H, w, u, vt);
    cv::Mat R = vt.t() * u.t();

    float scale = 1.0f;
    if (cv::determinant(R) < 0) {
        vt.row(1) *= -1;
        R = vt.t() * u.t();
    }

    float src_var = 0;
    for (const auto& pt : src_demean)
        src_var += pt.dot(pt);
    src_var /= num_points;

    scale = static_cast<float>(cv::sum(w)[0]) / src_var;

    cv::Mat T = cv::Mat::eye(3, 3, CV_32F);
    cv::Mat M = scale * R;
    M.copyTo(T(cv::Rect(0, 0, 2, 2)));

    cv::Mat src_center_mat = (cv::Mat_<float>(2, 1) << src_center.x, src_center.y);
    cv::Mat dst_center_mat = (cv::Mat_<float>(2, 1) << dst_center.x, dst_center.y);
    cv::Mat trans = dst_center_mat - M * src_center_mat;
    T.at<float>(0, 2) = trans.at<float>(0);
    T.at<float>(1, 2) = trans.at<float>(1);

    return T;
}

cv::Mat align_face(const cv::Mat& image, const FaceDetectionResult& faceBox) {
    if (faceBox.landmarks.size() < 4) return image;

    // Source points: eye_right, eye_left, nose, mouth
//    std::vector<cv::Point2f> src_pts = {
//        faceBox.landmarks[0],
//        faceBox.landmarks[1],
//        faceBox.landmarks[2],
//        faceBox.landmarks[3]
//    };
    std::vector<cv::Point2f> src_pts = {
        faceBox.landmarks[0],
        faceBox.landmarks[1],
        faceBox.landmarks[2],
        faceBox.landmarks[3],
        faceBox.landmarks[4],
        faceBox.landmarks[5]
    };

    // Target points in normalized [0,1] space
//    std::vector<cv::Point2f> dst_pts = {
//        {0.39f, 0.50f},
//        {0.61f, 0.50f},
//        {0.50f, 0.65f},
//        {0.50f, 0.75f}
//    };
    
    std::vector<cv::Point2f> dst_pts = {
        {0.3410f, 0.4600f},  // left eye
        {0.6550f, 0.4600f},  // right eye
        {0.4980f, 0.6100f},  // nose
        {0.4980f, 0.7500f},  // mouth center
        {0.2000f, 0.5000f},  // left ear
        {0.8000f, 0.5000f}   // right ear
    };
    

    int out_size = 112; // Auraface expects a 112 px image
    for (auto& pt : dst_pts) {
        pt.x *= out_size;
        pt.y *= out_size;
    }

    cv::Mat transform = similarity_transform(src_pts, dst_pts);
    cv::Mat aligned;
    cv::warpPerspective(image, aligned, transform, cv::Size(out_size, out_size), cv::INTER_LINEAR, cv::BORDER_REPLICATE);
    
    // Normalize for AuraFace
    // NOT DONE ANYMORE - DONE DIRECTLY INSIDE extract_embedding()
//    cv::Mat normalized;
//    normalize_to_11(aligned, normalized);
//    return normalized;
    return aligned;
}
