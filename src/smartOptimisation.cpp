/*
* Copyright (c) 2015 
* Guillaume Lemaitre (g.lemaitre58@gmail.com)
* Yohan Fougerolle (Yohan.Fougerolle@u-bourgogne.fr)
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation; either version 2 of the License, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc., 51
* Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "smartOptimisation.h"

// stl library
#include <vector>

// own library
#include "imageProcessing.h"

namespace initoptimisation {

  // Function to find normalisation factor
  double find_normalisation_factor(std::vector < cv::Point2f > contour) {

    // No method is available to find the max coordinate
    cv::Mat contour_x = cv::Mat::zeros(1, contour.size(), CV_32F);
    cv::Mat contour_y = cv::Mat::zeros(1, contour.size(), CV_32F);
    for (unsigned int contour_point_idx = 0; contour_point_idx < contour.size(); contour_point_idx++) {
      contour_x.at<float>(contour_point_idx) = std::abs(contour[contour_point_idx].x);
      contour_y.at<float>(contour_point_idx) = std::abs(contour[contour_point_idx].y);
    }
    
    double max_x, max_y;
    cv::minMaxIdx(contour_x, NULL, &max_x, NULL, NULL);
    cv::minMaxIdx(contour_y, NULL, &max_y, NULL, NULL);
    
    return std::max(max_x, max_y);
  }

  // Function to normalize a contour
  std::vector< cv::Point2f > normalise_contour(std::vector < cv::Point2f > contour, double &factor) {
    
    // Find the normalisation factor
    factor = find_normalisation_factor(contour);

    // Initilisation of the output
    std::vector< cv::Point2f > output_contour(contour.size());

    // Make the normalisation
    for (unsigned int contour_point_idx = 0; contour_point_idx < contour.size(); contour_point_idx++)
      output_contour[contour_point_idx] = contour[contour_point_idx] * (1 / factor);

    return output_contour;
  }

  // Function to normalise a vector of contours
  std::vector< std::vector< cv::Point2f > > normalise_all_contours(std::vector< std::vector < cv::Point2f > > contours, std::vector< double > &factor_vector) {
    
    // Allocate the output contours
    std::vector< std::vector< cv::Point2f > > output_contours(contours.size());

    // For each contour
    for (unsigned int contour_idx = 0; contour_idx < contours.size(); contour_idx++)
      output_contours[contour_idx] = normalise_contour(contours[contour_idx], factor_vector[contour_idx]);

    return output_contours;
  }

  // Function to denormalize a contour
  std::vector< cv::Point2f > denormalise_contour(std::vector < cv::Point2f > contour, double factor) {
    
    // Initilisation of the output
    std::vector< cv::Point2f > output_contour(contour.size());

    // Make the normalisation
    for (unsigned int contour_point_idx = 0; contour_point_idx < contour.size(); contour_point_idx++)
      output_contour[contour_point_idx] = contour[contour_point_idx] * factor;

    return output_contour;
  }

  // Function to denormalise a vector of contours
  std::vector< std::vector< cv::Point2f > > denormalise_all_contours(std::vector< std::vector < cv::Point2f > > contours, std::vector< double > factor_vector) {
    
    // Allocate the output contours
    std::vector< std::vector< cv::Point2f > > output_contours(contours.size());

    // For each contour
    for (unsigned int contour_idx = 0; contour_idx < contours.size(); contour_idx++)
      output_contours[contour_idx] = denormalise_contour(contours[contour_idx], factor_vector[contour_idx]);

    return output_contours;
  }

  // Function to estimate the radius for a contour - THE CONTOUR NEEDS TO BE DENORMALIZED
  int radius_estimation(std::vector< cv::Point2f > contour) { 
    
    double radius = 0;
    // Add the distance of each contour point
    for (unsigned int contour_point_idx = 0; contour_point_idx < contour.size(); contour_point_idx++) 
      radius += cv::norm(contour[contour_point_idx]);

    return (int) std::ceil(radius / (double)contour.size());
  }

  // Function to extract a ROI from one image with border copy if the ROI is too large
  cv::Mat roi_extraction(cv::Mat original_image, cv::Rect roi) {

    // Allocate the ouput image
    cv::Mat output_image;
    // No padding needed
    if ((roi.x > 0) && ((roi.x + roi.width) < original_image.cols) &&
	(roi.y > 0) && ((roi.y + roi.height) < original_image.rows) )
      output_image = original_image(roi);
    // Otherwise we need to pad arounf the image
    else {

      // Create a ROI with the part which is inside the original picture
      cv::Rect within_roi(roi);
      if (roi.x < 0) {
	within_roi.x = 0;
	within_roi.width -= roi.x;
      }
      if (roi.y < 0) {
	within_roi.y = 0;
	within_roi.height -= roi.y;
      }
      if ((within_roi.x + within_roi.width) >= original_image.cols)
	within_roi.width = original_image.cols - within_roi.x;
      if ((within_roi.y + within_roi.height) >= original_image.rows)
	within_roi.height = original_image.rows - within_roi.y;

      // Crop the ROI within the image
      cv::Mat within_image = original_image(within_roi);

      // Now create pad around the image with replication
      int top = 0, bottom = 0, left = 0, right = 0;
      if (roi.x < 0)
	left = std::abs(roi.x);
      if (roi.y < 0)
        top = std::abs(roi.y);
      if ((roi.x + roi.width) >= original_image.cols)
	right = roi.width - (original_image.cols - roi.x);
      if ((roi.y + roi.height) >= original_image.rows)
	bottom = roi.height - (original_image.rows - roi.y);

      // Pad the image
      cv::copyMakeBorder(within_image, output_image, top, bottom, left, right, cv::BORDER_REPLICATE);
    }
    
    return output_image;
  }

  // Function to return max and min in x and y of contours
  void extract_min_max(std::vector< cv::Point2f > contour, double &min_y, double &min_x, double &max_x, double &max_y) { 
    // Find the minimum coordinate around the supposed target
    // No method is available to find the max coordinate
    cv::Mat contour_x = cv::Mat::zeros(1, contour.size(), CV_32F);
    cv::Mat contour_y = cv::Mat::zeros(1, contour.size(), CV_32F);
    for (unsigned int contour_point_idx = 0; contour_point_idx < contour.size(); contour_point_idx++) {
      contour_x.at<float>(contour_point_idx) = contour[contour_point_idx].x;
      contour_y.at<float>(contour_point_idx) = contour[contour_point_idx].y;
    }
    cv::minMaxIdx(contour_x, &min_x, &max_x, NULL, NULL);
    cv::minMaxIdx(contour_y, &min_y, &max_y, NULL, NULL);
  }

  // Function to define the ROI dimension around a target by a given factor
  cv::Rect roi_dimension_definition(double min_y, double min_x, double max_x, double max_y, double factor) {
    
    // Create a RoI
    cv::Rect roi_dimension;
    roi_dimension.height = (int) ceil(factor * (max_y - min_y));
    roi_dimension.width = (int)  ceil(factor * (max_x - min_x));
    roi_dimension.x = (int) ceil(min_x + (1 - factor) * ((max_x - min_x) * 0.5));
    roi_dimension.y = (int) ceil(min_y + (1 - factor) * ((max_y - min_y) * 0.5));

    return roi_dimension;
  }

  // Function to convert the RGB to float gray
  cv::Mat rgb_to_float_gray(cv::Mat original_image) {

    // The roi image has to be converted in RGB for further processing
    cv::Mat gray_image;
    cv::cvtColor(original_image, gray_image, CV_RGB2GRAY);

    // Convert the image into float
    cv::Mat gray_image_float;
    gray_image.convertTo(gray_image_float, CV_32F); 

    return gray_image_float;
  }

  // Function which threshold the gradient image based on the magnitude image
  void gradient_thresh(cv::Mat &magnitude_image, cv::Mat &gradient_x, cv::Mat& gradient_y) {
    
    // Find the maximum magnitude
    double max_magnitude;
    cv::minMaxLoc(magnitude_image, NULL, &max_magnitude);
    
    for (int i = 0; i < magnitude_image.rows; i++) {
      for (int j = 0; j < magnitude_image.cols; j++) {
	if (magnitude_image.at<float>(i, j) < ((float) max_magnitude * THRESH_GRAD_RAD_DET)) {
	  gradient_x.at<float>(i, j) = 0.00;
	  gradient_y.at<float>(i, j) = 0.00;
	  magnitude_image.at<float>(i, j) = 0.00;
	}
      }
    }

  }

  // Function to determine the angles from the gradient images
  void orientations_from_gradient(cv::Mat gradient_x, cv::Mat gradient_y, int edges_number, cv::Mat &gradient_vp_x, cv::Mat &gradient_vp_y, cv::Mat &gradient_bar_x, cv::Mat &gradient_bar_y) {

    // Allocation of the diffrent gradients
    cv::Mat gradient_gp_radian = cv::Mat(gradient_x.size(), CV_32F);
    cv::Mat gradient_gp_degree = cv::Mat(gradient_x.size(), CV_32F);
    cv::Mat gradient_vp_degree = cv::Mat(gradient_x.size(), CV_32F);

    // Compute gradient gp in radian
    for (int i = 0; i < gradient_gp_radian.rows; i++)
      for (int j = 0; j < gradient_gp_radian.cols ; j++)
        gradient_gp_radian.at<float>(i, j) = atan2(gradient_y.at<float>(i, j), gradient_x.at<float>(i, j));

    // Convert from gradient gp to degree
    cv::divide((180.0 * gradient_gp_radian), cv::Mat::ones(gradient_gp_radian.size(), CV_32F) * M_PI, gradient_gp_degree);
     
    // Compute the gradient vp in degree
    for (int i = 0; i < gradient_vp_degree.rows; i++) {
      for (int j = 0; j < gradient_vp_degree.cols; j++) {
        gradient_vp_degree.at<float>(i, j) = gradient_gp_degree.at<float>(i, j) * (float) edges_number;
        gradient_vp_degree.at<float>(i, j) = fmod(gradient_vp_degree.at<float>(i, j), (float) 360.00);
      }
    }
    
    // Compute the angle difference between the gradients vp and gp
    cv::Mat theta;
    cv::divide((gradient_vp_degree - gradient_gp_degree) * M_PI, cv::Mat::ones(gradient_gp_degree.size(), CV_32F) * 180.0, theta);

    cv::Mat cos_theta = cv::Mat::zeros(theta.size(), CV_32F);
    cv::Mat sin_theta = cv::Mat::zeros(theta.size(), CV_32F);
    for (int i = 0; i < theta.rows; i++) {
      for (int j = 0; j < theta.cols; j++) {
        cos_theta.at<float>(i, j) = cos(theta.at<float>(i, j));
        sin_theta.at<float>(i, j) = sin(theta.at<float>(i, j));
      }
    }

    cv::Mat tmp_matrix_1;
    cv::Mat tmp_matrix_2;

    cv::multiply(cos_theta, gradient_x, tmp_matrix_1);
    cv::multiply(sin_theta, gradient_y, tmp_matrix_2);
    cv::subtract(tmp_matrix_1, tmp_matrix_2, gradient_vp_x);
    cv::multiply(sin_theta, gradient_x, tmp_matrix_1);
    cv::multiply(cos_theta, gradient_y, tmp_matrix_2);
    cv::add(tmp_matrix_1, tmp_matrix_2, gradient_vp_y);

    gradient_bar_x = gradient_y;
    gradient_bar_y = - gradient_x;
  }

  // Function to round a matrix
  cv::Mat round_matrix(cv::Mat original_matrix) {
    
    // Allocate the ouput
    cv::Mat result(original_matrix.size(), CV_32F);

    for(int i = 0; i< original_matrix.rows; i++) {
      const float* ptr_original = original_matrix.ptr<float>(i);
      float* ptr_result = result.ptr<float>(i);
      for(int j = 0; j < original_matrix.cols; j++ ) {
	ptr_result[j] = (float) cvRound(ptr_original[j]);
      }
    }

    return result;
  }

  // Function to determine mass center by voting
  cv::Point2f mass_center_by_voting(cv::Mat magnitude_image, cv::Mat gradient_x, cv::Mat gradient_y, cv::Mat gradient_bar_x, cv::Mat gradient_bar_y, cv::Mat gradient_vp_x, cv::Mat gradient_vp_y, float radius, int edges_number) {

    // Create all the possible combination of coordinate 
    cv::Mat coord_x = cv::Mat(magnitude_image.size(), CV_32F);
    cv::Mat coord_y = cv::Mat(magnitude_image.size(), CV_32F);
    for(int i = 0; i < coord_x.rows; i++) {
      float* ptr_coord_x = coord_x.ptr<float>(i);
      float* ptr_coord_y = coord_y.ptr<float>(i);
      for(int j = 0; j < coord_x.cols; j++) {
	ptr_coord_x[j] = (float) j;
	ptr_coord_y[j] = (float) i;
      }
    }
    
    // Allocate the different image needed during the voting process
    cv::Mat pos_vote_x;
    cv::Mat pos_vote_y;
    cv::Mat neg_vote_x;
    cv::Mat neg_vote_y;

    cv::Mat Or = cv::Mat::zeros(magnitude_image.size(), CV_32F);
    cv::Mat Br = cv::Mat::zeros(magnitude_image.size(), CV_32F);
    cv::Mat BrX = cv::Mat::zeros(magnitude_image.size(), CV_32F);
    cv::Mat BrY = cv::Mat::zeros(magnitude_image.size(), CV_32F);

    // Determine the coordinates of the positively and negatively affected pixels
    // For the positive part
    cv::add(coord_x, round_matrix(radius * gradient_x), pos_vote_x);
    cv::add(coord_y, round_matrix(radius * gradient_y), pos_vote_y);
    // For the negative part
    cv::subtract(coord_x, round_matrix(radius * gradient_x), neg_vote_x);
    cv::subtract(coord_y, round_matrix(radius * gradient_y), neg_vote_y);

    // Check if the values are inside the boundaries
    for(int i = 0; i < pos_vote_x.rows; i++) {
      float* ptr_pos_vote_x = pos_vote_x.ptr<float>(i);
      float* ptr_pos_vote_y = pos_vote_y.ptr<float>(i);
      float* ptr_neg_vote_x = neg_vote_x.ptr<float>(i);
      float* ptr_neg_vote_y = neg_vote_y.ptr<float>(i);

      for(int j = 0; j < pos_vote_x.cols; j++) {
	if(ptr_pos_vote_x[j] < 1) ptr_pos_vote_x[j] = 1;
	if(ptr_pos_vote_y[j] < 1) ptr_pos_vote_y[j] = 1;
	if(ptr_neg_vote_x[j] < 1) ptr_neg_vote_x[j] = 1;
	if(ptr_neg_vote_y[j] < 1) ptr_neg_vote_y[j] = 1;
	if(ptr_pos_vote_x[j] > pos_vote_x.cols - 1) ptr_pos_vote_x[j] = pos_vote_x.cols - 1;	
	if(ptr_pos_vote_y[j] > pos_vote_y.rows - 1) ptr_pos_vote_y[j] = pos_vote_y.rows - 1;
	if(ptr_neg_vote_x[j] > neg_vote_x.cols - 1) ptr_neg_vote_x[j] = neg_vote_x.cols - 1;	
	if(ptr_neg_vote_y[j] > neg_vote_y.rows - 1) ptr_neg_vote_y[j] = neg_vote_y.rows - 1;
      }
    }

    // Calculate W, the unit length of the vote lines in pixel
    int W = (int) ceil(radius * std::tan(M_PI / (float) edges_number));

    //Compute Votes
    for (int i = 0; i < magnitude_image.rows; i++) {
      for (int j = 0; j < magnitude_image.cols; j++) {
	if (magnitude_image.at<float>(i, j) != 0.00) {
	  // Positive votes
	  for (int m = - W; m <= W; m++) {
	    int LXpos = (int) pos_vote_x.at<float>(i, j) + (int) ceil((float) m * gradient_bar_x.at<float>(i, j));
	    int LYpos = (int) pos_vote_y.at<float>(i, j) + (int) ceil((float) m * gradient_bar_y.at<float>(i, j));

	    int LXneg = (int) neg_vote_x.at<float>(i, j) + (int) ceil((float) m * gradient_bar_x.at<float>(i, j));
	    int LYneg = (int) neg_vote_y.at<float>(i, j) + (int) ceil((float) m * gradient_bar_y.at<float>(i, j));

	    if((LXpos >= 0) && (LXpos < magnitude_image.cols) &&
	       (LYpos >= 0) && (LYpos < magnitude_image.rows)) {
	      // Vote for Or image
	      Or.at<float>(LYpos, LXpos) = Or.at<float>(LYpos, LXpos) + 1.00;

	      // Vote the Br image for X and Y
	      BrX.at<float>(LYpos, LXpos) = BrX.at<float>(LYpos, LXpos) + gradient_vp_x.at<float>(i, j);
	      BrY.at<float>(LYpos, LXpos) = BrY.at<float>(LYpos, LXpos) + gradient_vp_y.at<float>(i, j);
	    }
	    if((LXneg >= 0) && (LXneg < magnitude_image.cols) && 
	       (LYneg >= 0) && (LYneg < magnitude_image.rows)) {
	      // Vote for Or image
	      Or.at<float>(LYneg, LXneg) = Or.at<float>(LYneg, LXneg) + 1.00;

	      // Vote the Br image for X and Y
	      BrX.at<float>(LYneg, LXneg) = BrX.at<float>(LYneg, LXneg) + gradient_vp_x.at<float>(i, j);
	      BrY.at<float>(LYneg, LXneg) = BrY.at<float>(LYneg, LXneg) + gradient_vp_y.at<float>(i, j);
	    }
	  }

	  // First negative votes
	  for (int m = (- 2 * W); m <= (- W - 1); m++) {
	    int LXpos = (int) pos_vote_x.at<float>(i, j) + (int) ceil((float) m * gradient_bar_x.at<float>(i, j));
	    int LYpos = (int) pos_vote_y.at<float>(i, j) + (int) ceil((float) m * gradient_bar_y.at<float>(i, j));

	    int LXneg = (int) neg_vote_x.at<float>(i, j) + (int) ceil((float) m * gradient_bar_x.at<float>(i, j));
	    int LYneg = (int) neg_vote_y.at<float>(i, j) + (int) ceil((float) m * gradient_bar_y.at<float>(i, j));

	    if((LXpos >= 0) && (LXpos < magnitude_image.cols) && 
	       (LYpos >= 0) && (LYpos < magnitude_image.rows)) {
	      // Vote for Or image
	      Or.at<float>(LYpos, LXpos) = Or.at<float>(LYpos, LXpos) - 1.00;

	      // Vote the Br image for X and Y
	      BrX.at<float>(LYpos, LXpos) = BrX.at<float>(LYpos, LXpos) - gradient_vp_x.at<float>(i, j);
	      BrY.at<float>(LYpos, LXpos) = BrY.at<float>(LYpos, LXpos) - gradient_vp_y.at<float>(i, j);
	    }
	    if((LXneg >= 0) && (LXneg < magnitude_image.cols) && 
	       (LYneg >= 0) && (LYneg < magnitude_image.rows)) {
	      // Vote for Or image
	      Or.at<float>(LYneg, LXneg) = Or.at<float>(LYneg, LXneg) - 1.00;

	      // Vote the Br image for X and Y
	      BrX.at<float>(LYneg, LXneg) = BrX.at<float>(LYneg, LXneg) - gradient_vp_x.at<float>(i, j);
	      BrY.at<float>(LYneg, LXneg) = BrY.at<float>(LYneg, LXneg) - gradient_vp_y.at<float>(i, j);
	    }
	  }

	  // Second negative votes
	  for (int m = (W + 1); m <= (2 * W); m++) {
	    int LXpos = (int) pos_vote_x.at<float>(i, j) + (int) ceil((float) m * gradient_bar_x.at<float>(i, j));
	    int LYpos = (int) pos_vote_y.at<float>(i, j) + (int) ceil((float) m * gradient_bar_y.at<float>(i, j));

	    int LXneg = (int) neg_vote_x.at<float>(i, j) + (int) ceil((float) m * gradient_bar_x.at<float>(i, j));
	    int LYneg = (int) neg_vote_y.at<float>(i, j) + (int) ceil((float) m * gradient_bar_y.at<float>(i, j));

	    if((LXpos >= 0) && (LXpos < magnitude_image.cols) &&
	       (LYpos >= 0) && (LYpos < magnitude_image.rows)) {
	      // Vote for Or image
	      Or.at<float>(LYpos, LXpos) = Or.at<float>(LYpos, LXpos) - 1.00;

	      // Vote the Br image for X and Y
	      BrX.at<float>(LYpos, LXpos) = BrX.at<float>(LYpos, LXpos) - gradient_vp_x.at<float>(i, j);
	      BrY.at<float>(LYpos, LXpos) = BrY.at<float>(LYpos, LXpos) - gradient_vp_y.at<float>(i, j);
	    }
	    if((LXneg >= 0) && (LXneg < magnitude_image.cols) && 
	       (LYneg >= 0) && (LYneg < magnitude_image.rows)) {
	      // Vote for Or image
	      Or.at<float>(LYneg, LXneg) = Or.at<float>(LYneg, LXneg) - 1.00;

	      // Vote the Br image for X and Y
	      BrX.at<float>(LYneg, LXneg) = BrX.at<float>(LYneg, LXneg) - gradient_vp_x.at<float>(i, j);
	      BrY.at<float>(LYneg, LXneg) = BrY.at<float>(LYneg, LXneg) - gradient_vp_y.at<float>(i, j);
	    }
	  }
	}
      }
    }

    // Compute Br
    cv::magnitude(BrX, BrY, Br);

    // To avoid edge effect - remove 2 pixels of the image
    int border = 5;
    // For left border
    for (int j = 0; j < border; j++) {
      for (int i = 0; i < magnitude_image.rows; i++) {
	Or.at<float>(i, j) = 0.00;
	Br.at<float>(i, j) = 0.00;
      }
    }
    // For top border
    for (int i = 0; i < border; i++) {
      for (int j = 0; j < magnitude_image.cols; j++) {
	Or.at<float>(i, j) = 0.00;
	Br.at<float>(i, j) = 0.00;
      }
    }
    // For bottom border
    for (int i = magnitude_image.rows - 1; i <= magnitude_image.rows - border; i++) {
      for (int j = 0; j < magnitude_image.cols; j++) {
	Or.at<float>(i, j) = 0.00;
	Br.at<float>(i, j) = 0.00;
      }
    }
    // For the right border
    for (int j = magnitude_image.cols - 1; j <= magnitude_image.cols - border; j++) {
      for (int i = 0; i < magnitude_image.rows; i++) {
	Or.at<float>(i, j) = 0.00;
	Br.at<float>(i, j) = 0.00;
      }
    }
    
    // Allocate the image for the output
    cv::Mat Sr;
    cv::Mat S = cv::Mat::zeros(gradient_x.size(), CV_32F);

    if (edges_number == 12) 
      cv::multiply(Or, Or, Sr);
    else
      cv::multiply(Or, Br, Sr);
    
    cv::divide(Sr, cv::Mat::ones(magnitude_image.size(), CV_32F) * pow(2.00 * (float) W * radius, 2.00), Sr);

    double sigma = 0.2 * radius;
    int mask_size = (int) ceil(6 * sigma);
    if(!(mask_size % 2)) mask_size++;
    if(mask_size < 1) mask_size = 1;
    // Smooth Sr
    cv::Mat Sr_blurred;
    cv::GaussianBlur(Sr, Sr_blurred, cv::Size(mask_size, mask_size), sigma, sigma, cv::BORDER_CONSTANT);
    
    // Normalise Sr
    cv::Mat Sr_norm;
    cv::normalize(Sr_blurred, Sr_norm, 0.0, 1.0, cv::NORM_MINMAX);
    cv::add(S, Sr_norm, S);
    
    // Find the maximum intensity value
    double max_value;
    cv::minMaxLoc(S, NULL, &max_value);

    // Choose the threshold as close as possible to the maximum value
    float thresholdBin = (float) (max_value * THRESH_BINARY);

    // Convert to binary to extract the centers clearly
    cv::Mat S_thresh = S > thresholdBin;

    // Compute the gravity center
    float sumX = 0, sumY = 0, normalization = 0;
    for (int i = 0; i < S_thresh.rows; i ++) {
      for (int j = 0; j < S_thresh.cols; j++) {
	if (S_thresh.at<bool>(i,j) == true) {
	  sumX += (float) j;
	  sumY += (float) i;
	  normalization += 1.00;
	}
      }
    }

    return(cv::Point2f(ceil(sumX / normalization), ceil(sumY / normalization)));
  }

  // Function to discover the mass center using the radial symmetry detector
  cv::Point2f radial_symmetry_detector(cv::Mat roi_image, int radius, int edges_number) {
    
    /*
     * Conversion to write data type
     */

    cv::Mat gray_image_float = rgb_to_float_gray(roi_image);
   
    // Apply a Gaussian filtering
    cv::Mat blurred_image;
    cv::GaussianBlur(gray_image_float, blurred_image, cv::Size(3,3), 0, 0, cv::BORDER_DEFAULT);

    /*
     * Gradients computation before voting
     */    

    // Compute the gardient image
    // Define the kernel to apply to the image
    cv::Mat kernel_x = cv::Mat(5, 5, CV_32F, derivative_x);
    cv::Mat kernel_y = cv::Mat(5, 5, CV_32F, derivative_y);

    // Filter the image to compute the gradient
    cv::Mat gradient_x, gradient_y;
    cv::filter2D(blurred_image, gradient_x, CV_32F, - kernel_x);
    cv::filter2D(blurred_image, gradient_y, CV_32F, - kernel_y);

    // Compute the magnitude
    cv::Mat magnitude_image;
    cv::magnitude(gradient_x, gradient_y, magnitude_image);

    // Normalise the gradient image using the magnitude
    cv::divide(gradient_x, magnitude_image, gradient_x);
    cv::divide(gradient_y, magnitude_image, gradient_y);
    
    /*
     * Gradients filtering
     */

    gradient_thresh(magnitude_image, gradient_x, gradient_y);

    /*
     * Orientation computation
     */

    cv::Mat gradient_vp_x, gradient_vp_y, gradient_bar_x, gradient_bar_y;
    orientations_from_gradient(gradient_x, gradient_y, edges_number, gradient_vp_x, gradient_vp_y, gradient_bar_x, gradient_bar_y);
    
    return mass_center_by_voting(magnitude_image, gradient_x, gradient_y, gradient_bar_x, gradient_bar_y, gradient_vp_x, gradient_vp_y, (float) radius, edges_number);
  }
  
  // Function to discover an approximation of the mass center for each contour using a voting method for a given contour
  cv::Point2f mass_center_discovery(cv::Mat original_image, cv::Mat translation_matrix, cv::Mat rotation_matrix, cv::Mat scaling_matrix, std::vector< cv::Point2f > contour, double factor, int type_traffic_sign) {

    // Compute the transformation necessary to warp the original image
    cv::Mat transform_warping = translation_matrix.inv() * rotation_matrix * scaling_matrix * translation_matrix;
    
    // Warp the original image
    cv::Mat warp_image;
    cv::warpPerspective(original_image, warp_image, transform_warping, original_image.size(), cv::INTER_CUBIC, cv::BORDER_REPLICATE);

    // We need to denormalise the contour using the normalisation factor
    std::vector< cv::Point2f > denormalised_contour = denormalise_contour(contour, factor);

    // Estimate the radius given a contour
    int radius_contour = radius_estimation(denormalised_contour);

    // We need to inverse the translation
    std::vector< cv::Point2f > denormalised_contour_no_translation = imageprocessing::inverse_transformation_contour(denormalised_contour, translation_matrix);

    // Find the minimum coordinate around the supposed target
    double min_y, min_x, max_y, max_x;
    extract_min_max(denormalised_contour_no_translation, min_y, min_x, max_x, max_y);

    // Define a ROI around the supposed target
    cv::Rect roi_dimension = roi_dimension_definition(min_y, min_x, max_x, max_y, 2.0);

    // ROI extraction
    cv::Mat roi_image = roi_extraction(warp_image, roi_dimension);

    // The main function need to know how many edges each traffic sign as
    int edges_number;
    switch (type_traffic_sign) {
    case 0:
      edges_number = 3;
      break;
    case 1:
      edges_number = 4;
      break;
    case 2:
      edges_number = 12;
      break;
    case 3:
      edges_number = 8;
      break;
    case 4:
      edges_number = 3;
      break;
    }

    return radial_symmetry_detector(roi_image, radius_contour, edges_number);
  }

}
