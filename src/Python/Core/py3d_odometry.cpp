// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2017 Jaesik Park <syncle@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "py3d_core.h"
#include "py3d_core_trampoline.h"

#include <Core/Odometry/Odometry.h>
#include <Core/Odometry/OdometryOption.h>
#include <Core/Odometry/RGBDOdometryJacobian.h>
using namespace three;

typedef std::tuple<Eigen::MatrixXd, Eigen::VectorXd> JacobianResidual;

template <class RGBDOdometryJacobianBase = RGBDOdometryJacobian>
class PyRGBDOdometryJacobian : public RGBDOdometryJacobianBase
{
public:
	using RGBDOdometryJacobianBase::RGBDOdometryJacobianBase;
	std::tuple<Eigen::MatrixXd, Eigen::VectorXd> ComputeJacobian(
			const RGBDImage &source, const RGBDImage &target,
			const Image &source_xyz,
			const RGBDImage &target_dx, const RGBDImage &target_dy,
			const Eigen::Matrix4d &odo,
			const CorrespondenceSetPixelWise &corresps,
			const Eigen::Matrix3d &camera_matrix,
			const OdometryOption &option) const override {
			PYBIND11_OVERLOAD_PURE(
			JacobianResidual,
			RGBDOdometryJacobianBase,
			source, target, source_xyz, target_dx, target_dy,
			odo, corresps, camera_matrix, option);
	}
};

void pybind_odometry(py::module &m)
{
	py::class_<OdometryOption> odometry_option(m, "OdometryOption");
	odometry_option.def("__init__", [](OdometryOption &c,
		double minimum_correspondence_ratio,
		std::vector<int> iteration_number_per_pyramid_level, 
		double max_depth_diff, double min_depth, double max_depth) {
		new (&c)OdometryOption(minimum_correspondence_ratio,
				iteration_number_per_pyramid_level, 
				max_depth_diff, min_depth, max_depth);
	}, "minimum_correspondence_ratio"_a = 0.1, 
		"iteration_number_per_pyramid_level"_a = std::vector<int>{ 10,10,10,5 },
		"max_depth_diff"_a = 0.07, "min_depth"_a = 0.0, "max_depth"_a = 4.0);
	odometry_option
		.def_readwrite("minimum_correspondence_num",
				&OdometryOption::minimum_correspondence_ratio_)
		.def_readwrite("iteration_number_per_pyramid_level",
				&OdometryOption::iteration_number_per_pyramid_level_)
		.def_readwrite("max_depth_diff", &OdometryOption::max_depth_diff_)
		.def_readwrite("min_depth", &OdometryOption::min_depth_)
		.def_readwrite("max_depth", &OdometryOption::max_depth_)
		.def("__repr__", [](const OdometryOption &c) {
		int num_pyramid_level = c.iteration_number_per_pyramid_level_.size();
		std::string str_iteration_number_per_pyramid_level_ = "[ ";
		for (int i = 0; i < num_pyramid_level; i++) 
			str_iteration_number_per_pyramid_level_ += 
					std::to_string(c.iteration_number_per_pyramid_level_[i]) + ", ";
		str_iteration_number_per_pyramid_level_ += "] ";
		return std::string("OdometryOption class.") +
				/*std::string("\nodo_init = ") + std::to_string(c.odo_init_) +*/
				std::string("\nminimum_correspondence_ratio = ") +
				std::to_string(c.minimum_correspondence_ratio_) +
				std::string("\niteration_number_per_pyramid_level = ") +
				str_iteration_number_per_pyramid_level_ +
				std::string("\nmax_depth_diff = ") +
				std::to_string(c.max_depth_diff_) +
				std::string("\nmin_depth = ") +
				std::to_string(c.min_depth_) +
				std::string("\nmax_depth = ") +
				std::to_string(c.max_depth_);
		});

	py::class_<RGBDOdometryJacobian,
			PyRGBDOdometryJacobian<RGBDOdometryJacobian>>
			jacobian(m, "RGBDOdometryJacobian");
	jacobian
			.def("ComputeJacobian", &RGBDOdometryJacobian::ComputeJacobian);

	py::class_<RGBDOdometryJacobianfromColorTerm,
			PyRGBDOdometryJacobian<RGBDOdometryJacobianfromColorTerm>,
			RGBDOdometryJacobian> jacobian_color(m,
			"RGBDOdometryJacobianfromColorTerm");
	py::detail::bind_default_constructor<RGBDOdometryJacobianfromColorTerm>
			(jacobian_color);
	py::detail::bind_copy_functions<RGBDOdometryJacobianfromColorTerm>(
			jacobian_color);	
	jacobian_color
		.def("__repr__", [](const RGBDOdometryJacobianfromColorTerm &te) {
		return std::string("RGBDOdometryJacobianfromColorTerm");
	});

	py::class_<RGBDOdometryJacobianfromHybridTerm,
			PyRGBDOdometryJacobian<RGBDOdometryJacobianfromHybridTerm>,
			RGBDOdometryJacobian> jacobian_hybrid(m,
			"RGBDOdometryJacobianfromHybridTerm");
	py::detail::bind_default_constructor<RGBDOdometryJacobianfromHybridTerm>
		(jacobian_hybrid);
	py::detail::bind_copy_functions<RGBDOdometryJacobianfromHybridTerm>(
		jacobian_hybrid);
	jacobian_hybrid
		.def("__repr__", [](const RGBDOdometryJacobianfromHybridTerm &te) {
		return std::string("RGBDOdometryJacobianfromHybridTerm");
	});
}

void pybind_odometry_methods(py::module &m)
{
	m.def("ComputeRGBDOdometry", &ComputeRGBDOdometry,
			"Function to estimate 6D rigid motion from two RGBD image pairs",
			"rgbd_source"_a, "rgbd_target"_a, 
			"camera_intrinsic"_a = PinholeCameraIntrinsic(), 
			"odo_init"_a = Eigen::Matrix4d::Identity(),
			"jacobian"_a = RGBDOdometryJacobianfromHybridTerm(),
			"option"_a = OdometryOption());
}
