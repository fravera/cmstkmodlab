/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//               Copyright (C) 2011-2017 - The DESY CMS Group                  //
//                           All rights reserved                               //
//                                                                             //
//      The CMStkModLab source code is licensed under the GNU GPL v3.0.        //
//      You have the right to modify and/or redistribute this source code      //
//      under the terms specified in the license, which may be found online    //
//      at http://www.gnu.org/licenses or at License.txt.                      //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

#include <MarkerFinderPatRec.h>
#include <ApplicationConfig.h>
#include <nqlogger.h>
#include <Util.h>

#include <iostream>
#include <memory>

#include <TFile.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TH1.h>

int MarkerFinderPatRec::exe_counter_ = -1;

MarkerFinderPatRec::MarkerFinderPatRec(const QString& output_dir_path, const QString& output_subdir_name, QObject* parent) :
  QObject(parent),

  output_dir_path_   (output_dir_path   .toStdString()),
  output_subdir_name_(output_subdir_name.toStdString()),

  threshold_(-1),
  threshold_tpl_(-1),

  updated_threshold_(false),
  updated_image_master_(false),
  updated_image_master_binary_(false)
{
  // connections
  connect(this, SIGNAL(run_template_matching(const cv::Mat&, const cv::Mat&, const cv::Mat&, const int)),
          this, SLOT  (    template_matching(const cv::Mat&, const cv::Mat&, const cv::Mat&, const int)));
  // -----------

  NQLog("MarkerFinderPatRec", NQLog::Debug) << "constructed";
}

MarkerFinderPatRec::~MarkerFinderPatRec()
{
  NQLog("MarkerFinderPatRec", NQLog::Debug) << "destructed";
}

void MarkerFinderPatRec::set_threshold(const int v)
{
  if(threshold_ != v)
  {
    threshold_ = v;

    if(!updated_threshold_){ updated_threshold_ = true; }

    if(updated_image_master_binary_){ updated_image_master_binary_ = false; }
  }

  return;
}

void MarkerFinderPatRec::update_threshold(const int v)
{
  this->set_threshold(v);

  NQLog("MarkerFinderPatRec", NQLog::Debug) << "update_threshold(" << v << ")"
     << ": emitting signal \"threshold_updated\"";

  emit threshold_updated();
}

void MarkerFinderPatRec::acquire_image()
{
  NQLog("MarkerFinderPatRec", NQLog::Debug) << "acquire_image"
     << ": emitting signal \"image\"";

  emit image_request();
}

void MarkerFinderPatRec::update_image(const cv::Mat& img)
{
  image_mas_ = img;

  if(!updated_image_master_){ updated_image_master_ = true; }

  if(updated_image_master_binary_){ updated_image_master_binary_ = false; }

  NQLog("MarkerFinderPatRec", NQLog::Debug) << "update_image"
     << ": emitting signal \"image_updated\"";

  emit image_updated(image_mas_);
}

void MarkerFinderPatRec::delete_image()
{
  image_mas_ = cv::Mat();

  if(updated_image_master_){ updated_image_master_ = false; }

  return;
}

void MarkerFinderPatRec::update_binary_image()
{
  if(updated_image_master_)
  {
    if(!updated_threshold_)
    {
      NQLog("MarkerFinderPatRec", NQLog::Warning) << "update_binary_image"
         << ": threshold value not available, no binary image produced";

      return;
    }

    image_bin_ = this->get_binary_image(image_mas_, threshold_);

    if(!updated_image_master_binary_){ updated_image_master_binary_ = true; }

    NQLog("MarkerFinderPatRec", NQLog::Spam) << "update_binary_image"
       << ": created binary image with threshold=" << threshold_;

    NQLog("MarkerFinderPatRec", NQLog::Debug) << "update_binary_image"
       << ": emitting signal \"binary_image_updated\"";

    emit binary_image_updated(image_bin_);
  }
  else
  {
    NQLog("MarkerFinderPatRec", NQLog::Warning) << "update_binary_image"
       << ": master image not available, no binary image produced (hint: enable camera and get an image)";

    return;
  }
}

cv::Mat MarkerFinderPatRec::get_binary_image(const cv::Mat& img, const int threshold) const
{
  // greyscale image
  cv::Mat img_gs(img.size(), img.type());

  if(img.channels() > 1)
  {
    // convert color to GS
    cv::cvtColor(img, img_gs, CV_BGR2GRAY);
  }
  else
  {
    img_gs = img.clone();
  }

  // binary image (thresholding)
  cv::Mat img_bin(img_gs.size(), img_gs.type());
  cv::threshold(img_gs, img_bin, threshold, 255, cv::THRESH_BINARY);

  return img_bin;
}

void MarkerFinderPatRec::delete_binary_image()
{
  image_bin_ = cv::Mat();

  if(updated_image_master_binary_){ updated_image_master_binary_ = false; }

  return;
}

void MarkerFinderPatRec::run_PatRec(const int mode_lab, const int mode_obj)
{
  NQLog("MarkerFinderPatRec", NQLog::Message) << "run_PatRec"
     << "(mode_lab=" << mode_lab << ", mode_obj=" << mode_obj << ")"
     << ": initiated Pattern Recognition";

  // --- input validation
  if(!updated_threshold_)
  {
    NQLog("MarkerFinderPatRec", NQLog::Warning) << "run_PatRec"
       << "(mode_lab=" << mode_lab << ", mode_obj=" << mode_obj << ")"
       << ": threshold value not available, no action taken";

    return;
  }
  // --------------------

  if(mode_lab == 1) // PRODUCTION MODE
  {
    // --- input validation
    if(!updated_image_master_)
    {
      NQLog("MarkerFinderPatRec", NQLog::Warning) << "run_PatRec"
         << "(mode_lab=" << mode_lab << ", mode_obj=" << mode_obj << ")"
         << ": updated master image not available, no action taken";

      return;
    }
    // --------------------

    if(!updated_image_master_binary_){ this->update_binary_image(); }

    if(mode_obj == 0)
    {
      NQLog("MarkerFinderPatRec", NQLog::Spam) << "run_PatRec"
         << "(mode_lab=" << mode_lab << ", mode_obj=" << mode_obj << ")"
         << ": detection of sensor fiducial marker";

      image_tpl_ = cv::imread(Config::CMSTkModLabBasePath+"/share/assembly/SensorPiece_1_clipC.png"   , CV_LOAD_IMAGE_COLOR);
//    image_tpl_ = cv::imread(Config::CMSTkModLabBasePath+"/share/assembly/RawSensor_3_clipB.png"     , CV_LOAD_IMAGE_COLOR);
//    image_tpl_ = cv::imread(Config::CMSTkModLabBasePath+"/share/assembly/RawSensor_3_clipB_temp.png", CV_LOAD_IMAGE_COLOR);

      threshold_tpl_ = 85; // 90 for silicon marker, 88 for glass?
    }
    else if(mode_obj == 1)
    {
      NQLog("MarkerFinderPatRec", NQLog::Warning) << "run_PatRec"
         << "(mode_lab=" << mode_lab << ", mode_obj=" << mode_obj << ")"
         << ": detection of positioning pin not implemented yet, no action taken";

      return;
    }
    else if(mode_obj == 2)
    {
      NQLog("MarkerFinderPatRec", NQLog::Spam) << "run_PatRec"
         << "(mode_lab=" << mode_lab << ", mode_obj=" << mode_obj << ")"
         << ": detection of sensor corner";

      image_tpl_ = cv::imread(Config::CMSTkModLabBasePath+"/share/assembly/simplecorner.png"                                  , CV_LOAD_IMAGE_COLOR);
//    image_tpl_ = cv::imread(Config::CMSTkModLabBasePath+"/share/assembly/glassslidecorneronbaseplate_sliverpaint_A_clip.png", CV_LOAD_IMAGE_COLOR);
//    image_tpl_ = cv::imread(Config::CMSTkModLabBasePath+"/share/assembly/glassslidecorner_sliverpaint_D_crop.png"           , CV_LOAD_IMAGE_COLOR);

      threshold_tpl_ = 85; // 90 for silicon marker, 88 for glass?
    }
    else if(mode_obj == 3)
    {
      NQLog("MarkerFinderPatRec", NQLog::Spam) << "run_PatRec"
         << "(mode_lab=" << mode_lab << ", mode_obj=" << mode_obj << ")"
         << ": detection of spacer corner";

      image_tpl_ = cv::imread(Config::CMSTkModLabBasePath+"/share/assembly/spacer_corner_tempate_crop.png", CV_LOAD_IMAGE_COLOR);

      threshold_tpl_ = 85; // 90 for silicon marker, 88 for glass?
    }
    else
    {
      NQLog("MarkerFinderPatRec", NQLog::Warning) << "run_PatRec"
         << "(mode_lab=" << mode_lab << ", mode_obj=" << mode_obj << ")"
         << ": undefined value for object-mode, no action taken";

      return;
    }
  }
  else if(mode_lab == 0) // DEMO MODE
  {
    if(mode_obj == 0)
    {
      NQLog("MarkerFinderPatRec", NQLog::Spam) << "run_PatRec"
         << "(mode_lab=" << mode_lab << ", mode_obj=" << mode_obj << ")"
         << ": detection of sensor fiducial marker";

      image_mas_ = cv::imread(Config::CMSTkModLabBasePath+"/share/assembly/SensorPiece_1.png"      , CV_LOAD_IMAGE_COLOR);
      image_tpl_ = cv::imread(Config::CMSTkModLabBasePath+"/share/assembly/SensorPiece_1_clipC.png", CV_LOAD_IMAGE_COLOR);

      threshold_tpl_ = 85; // 90 for silicon marker, 88 for glass?
    }
    else if(mode_obj == 1)
    {
      NQLog("MarkerFinderPatRec", NQLog::Warning) << "run_PatRec"
         << "(mode_lab=" << mode_lab << ", mode_obj=" << mode_obj << ")"
         << ": detection of positioning pin not implemented yet, no action taken";

      return;
    }
    else if(mode_obj == 2)
    {
      NQLog("MarkerFinderPatRec", NQLog::Spam) << "run_PatRec"
         << "(mode_lab=" << mode_lab << ", mode_obj=" << mode_obj << ")"
         << ": detection of sensor corner";

      image_mas_ = cv::imread(Config::CMSTkModLabBasePath+"/share/assembly/glassslidecorneronbaseplate_sliverpaint_A.png"     , CV_LOAD_IMAGE_COLOR);
      image_tpl_ = cv::imread(Config::CMSTkModLabBasePath+"/share/assembly/glassslidecorneronbaseplate_sliverpaint_A_clip.png", CV_LOAD_IMAGE_COLOR);

      threshold_tpl_ = 85; // 90 for silicon marker, 88 for glass?
    }
    else if(mode_obj == 3)
    {
      NQLog("MarkerFinderPatRec", NQLog::Spam) << "run_PatRec"
         << "(mode_lab=" << mode_lab << ", mode_obj=" << mode_obj << ")"
         << ": detection of spacer corner not implemented, no action taken";

      return;
    }
    else
    {
      NQLog("MarkerFinderPatRec", NQLog::Warning) << "run_PatRec"
         << "(mode_lab=" << mode_lab << ", mode_obj=" << mode_obj << ")"
         << ": undefined value for object-mode, no action taken";

      return;
    }

    this->update_image(image_mas_);
    this->update_binary_image();
  }
  else
  {
    NQLog("MarkerFinderPatRec", NQLog::Warning) << "run_PatRec"
       << "(mode_lab=" << mode_lab << ", mode_obj=" << mode_obj << ")"
       << ": undefined value for lab-mode, no action taken";

    return;
  }

  NQLog("MarkerFinderPatRec", NQLog::Debug) << "run_PatRec"
     << "(mode_lab=" << mode_lab << ", mode_obj=" << mode_obj << ")"
     << ": emitting signal \"run_template_matching\"";

  emit run_template_matching(image_mas_, image_bin_, image_tpl_, threshold_tpl_);
}

void MarkerFinderPatRec::template_matching(const cv::Mat& img_master, const cv::Mat& img_master_bin, const cv::Mat& img_templa, const int threshold_templa)
{
    NQLog("MarkerFinderPatRec", NQLog::Debug) << "template_matching";
    NQLog("MarkerFinderPatRec", NQLog::Debug) << "template_matching: Master   cols = " << img_master.cols;
    NQLog("MarkerFinderPatRec", NQLog::Debug) << "template_matching: Master   rows = " << img_master.rows;
    NQLog("MarkerFinderPatRec", NQLog::Debug) << "template_matching: Template cols = " << img_templa.cols;
    NQLog("MarkerFinderPatRec", NQLog::Debug) << "template_matching: Template rows = " << img_templa.rows;

    // output directory
    std::string output_dir(""), output_subdir("");

    bool  output_dir_exists(true);
    while(output_dir_exists)
    {
      ++exe_counter_;

      std::string exe_counter_str = std::to_string(exe_counter_);

      if(exe_counter_ < 1e3)
      {
        std::stringstream exe_counter_strss;
        exe_counter_strss << std::setw(3) << std::setfill('0') << exe_counter_;
        exe_counter_str = exe_counter_strss.str();
      }

      output_dir = output_dir_path_+"/"+exe_counter_str+"/";

      output_dir_exists = Util::DirectoryExists(output_dir);
    }

    output_subdir = output_dir+output_subdir_name_;

    Util::QDir_mkpath(output_dir);

    NQLog("MarkerFinderPatRec", NQLog::Message) << "template_matching: created output directory: " << output_dir;

    Util::QDir_mkpath(output_subdir);

    NQLog("MarkerFinderPatRec", NQLog::Message) << "template_matching: created output directory: " << output_subdir;
    // -----------

    // GreyScale images
    cv::Mat img_master_gs(img_master.size(), img_master.type());
    cv::Mat img_templa_gs(img_templa.size(), img_templa.type());

    if(img_master.channels() > 1){ cv::cvtColor(img_master, img_master_gs, CV_BGR2GRAY); }
    else                         { img_master_gs = img_master.clone(); }

    if(img_templa.channels() > 1){ cv::cvtColor(img_templa, img_templa_gs, CV_BGR2GRAY); }
    else                         { img_templa_gs = img_templa.clone(); }
    // -----------

    // Binary images
    cv::Mat img_templa_bin(img_templa_gs.size(), img_templa_gs.type());

    cv::threshold(img_templa_gs, img_templa_bin, threshold_templa, 255, cv::THRESH_BINARY);

    const std::string filepath_img_master_bin = output_dir+"/image_master_binary.png";
    const std::string filepath_img_templa     = output_dir+"/image_template.png";
    const std::string filepath_img_templa_bin = output_dir+"/image_template_binary.png";

    cv::imwrite(filepath_img_master_bin, img_master_bin);

    NQLog("MarkerFinderPatRec", NQLog::Spam) << "template_matching"
       << ": saved master-binary image to " << filepath_img_master_bin;

    cv::imwrite(filepath_img_templa, img_templa);

    NQLog("MarkerFinderPatRec", NQLog::Spam) << "template_matching"
       << ": saved template image to " << filepath_img_templa;

    cv::imwrite(filepath_img_templa_bin, img_templa_bin);

    NQLog("MarkerFinderPatRec", NQLog::Spam) << "template_matching"
       << ": saved template-binary image to " << filepath_img_templa_bin;
    // -----------

    // --- Template Matching

    /* Template-Matching method for matchTemplate() routine of OpenCV
     * For SQDIFF and SQDIFF_NORMED, the best matches are lower values. For all the other methods, the higher the better.
     * REF https://docs.opencv.org/2.4/modules/imgproc/doc/object_detection.html?highlight=matchtemplate#matchtemplate
     */
    const int match_method = CV_TM_SQDIFF_NORMED;
    const bool use_minFOM = ((match_method  == CV_TM_SQDIFF) || (match_method == CV_TM_SQDIFF_NORMED));

    NQLog("MarkerFinderPatRec", NQLog::Spam) << "template_matching" << ": initiated matching routine with angular scan";

    // First, get theta-rough angle: best guess of central value for finer angular scan
    double theta_rough(-9999.);

    const std::vector<double> v_test_angles({0., 180.});
    {
      double best_FOM(0.);

      for(unsigned int i=0; i<v_test_angles.size(); ++i)
      {
        double i_angle = v_test_angles.at(i);

        double i_FOM(0.);
        cv::Point i_matchLoc;

        this->PatRec(i_FOM, i_matchLoc, img_master_bin, img_templa_bin, i_angle, match_method);

        const bool update = (i==0) || (use_minFOM ? (i_FOM < best_FOM) : (i_FOM > best_FOM));

        if(update){ best_FOM = i_FOM; theta_rough = i_angle; }
      }
    }

    NQLog("MarkerFinderPatRec", NQLog::Message) << "template_matching" << ": rough estimate of best-angle yields best-theta=" << theta_rough;
    // ----------------

    const double theta_fine_min  = -5.0;
    const double theta_fine_max  = +5.0;
    const double theta_fine_step =  0.25;

    NQLog("MarkerFinderPatRec", NQLog::Message) << "template_matching" << ": angular scan parameters"
       << "(min="<< theta_rough+theta_fine_min << ", max=" << theta_rough+theta_fine_max << ", step=" << theta_fine_step << ")";

    const int N_rotations = (2 * int((theta_fine_max-theta_fine_min) / theta_fine_step));

    std::vector<std::pair<double, double> > vec_angleNfom;
    vec_angleNfom.reserve(N_rotations);

    double    best_FOM  (0.);
    double    best_theta(0.);
    cv::Point best_matchLoc;

    for(double theta_fine=theta_fine_min; theta_fine<theta_fine_max; theta_fine += theta_fine_step)
    {
      const unsigned int scan_counter = vec_angleNfom.size();

      double i_theta = theta_rough + theta_fine;

      double i_FOM(0.);
      cv::Point i_matchLoc;

      this->PatRec(i_FOM, i_matchLoc, img_master_bin, img_templa_bin, i_theta, match_method, output_subdir);

      const bool update = (scan_counter==0) || (use_minFOM ? (i_FOM < best_FOM) : (i_FOM > best_FOM));

      if(update)
      {
        best_FOM      = i_FOM;
        best_theta    = i_theta;
        best_matchLoc = i_matchLoc;
      }

      vec_angleNfom.emplace_back(std::make_pair(i_theta, i_FOM));

      NQLog("MarkerFinderPatRec", NQLog::Spam) << "template_matching"
         << ": angular scan: [" << scan_counter << "] theta=" << i_theta << ", FOM=" << i_FOM;
    }

    NQLog("MarkerFinderPatRec", NQLog::Message) << "template_matching"
       << ": angular scan completed: best_theta=" << best_theta;

    cv::Mat img_master_copy = img_master.clone();

    line(img_master_copy, cv::Point(img_master_copy.cols/2.0, 0), cv::Point(img_master_copy.cols/2.0, img_master_copy.rows ), cv::Scalar(255,255,0), 2, 8, 0);
    line(img_master_copy, cv::Point(0, img_master_copy.rows/2.0), cv::Point(img_master_copy.cols, img_master_copy.rows/2.0 ), cv::Scalar(255,255,0), 2, 8, 0);

    if(theta_rough == 0.)
    {
      //the circle of radius 4 is meant to *roughly* represent the x,y precision of the x-y motion stage so that the
      //use can see if the patrec results make sense (the top left corner of the marker should be within the cirle)
      line(img_master_copy, cv::Point(best_matchLoc.x, best_matchLoc.y - 50), cv::Point(best_matchLoc.x + 240, best_matchLoc.y - 50), cv::Scalar(0, 255, 0), 2, 8, 0);

      putText(img_master_copy, "200 um", cv::Point(best_matchLoc.x, best_matchLoc.y - 100), cv::FONT_HERSHEY_SCRIPT_SIMPLEX, 2, cv::Scalar(0, 255, 0), 3, 8);

      circle(img_master_copy, best_matchLoc, 4, cv::Scalar(255, 0, 255), 4, 8, 0);

      rectangle(img_master_copy, best_matchLoc, cv::Point(best_matchLoc.x + img_templa_bin.cols, best_matchLoc.y + img_templa_bin.rows), cv::Scalar(255, 0, 255), 2, 8, 0);
    }
    else if(theta_rough == 180.)
    {
      line(img_master_copy, cv::Point(img_master_copy.cols - best_matchLoc.x - 240, img_master_copy.rows - best_matchLoc.y - 100),
                            cv::Point(img_master_copy.cols - best_matchLoc.x, img_master_copy.rows - best_matchLoc.y - 100), cv::Scalar(0, 255, 0), 2, 8, 0);

      putText(img_master_copy, "200 um", cv::Point(img_master_copy.cols - best_matchLoc.x - 240, img_master_copy.rows - best_matchLoc.y - 50), cv::FONT_HERSHEY_SCRIPT_SIMPLEX, 2, cv::Scalar(0, 255, 0), 3, 8);

      circle(img_master_copy, cv::Point(img_master_copy.cols - best_matchLoc.x, img_master_copy.rows-best_matchLoc.y), 4, cv::Scalar(255, 0, 255), 4, 8, 0);

      rectangle(img_master_copy, cv::Point(img_master_copy.cols - best_matchLoc.x, img_master_copy.rows - best_matchLoc.y),
                                 cv::Point(img_master_copy.cols - best_matchLoc.x - img_templa_bin.cols, img_master_copy.rows - best_matchLoc.y - img_templa_bin.rows), cv::Scalar(255, 0, 255), 2, 8, 0);
    }
    else
    {
      NQLog("MarkerFinderPatRec", NQLog::Warning) << "template_matching"
         << ": undefined behaviour for rough-theta=" << theta_rough << ", no lines/circles will be displayed on master image copy";
    }

    // FOM(angle) plot
    if(vec_angleNfom.size() > 0)
    {
      std::unique_ptr<TCanvas> c1(new TCanvas("FOM", "Rotation extraction", 200, 10, 700, 500));

      std::unique_ptr<TGraph> gr_scan(new TGraph());
      for(unsigned int idx=0; idx<vec_angleNfom.size(); ++idx)
      {
        gr_scan->SetPoint(idx, vec_angleNfom.at(idx).first, vec_angleNfom.at(idx).second);
      }

//      gr_scan->Fit("pol6");

      gr_scan->Draw("AC*");
      gr_scan->SetName("PatRec_FOM");
      gr_scan->GetHistogram()->GetXaxis()->SetTitle("angle (degrees)");
      gr_scan->GetHistogram()->GetYaxis()->SetTitle("PatRec FOM");
      gr_scan->GetHistogram()->SetTitle("");

      std::unique_ptr<TGraph> gr_best(new TGraph(1));
      gr_best->SetPoint(0, best_theta, best_FOM);
      gr_best->SetMarkerColor(2);
      gr_best->SetMarkerStyle(22);
      gr_best->SetMarkerSize(3);
      gr_best->Draw("PSAME");
      gr_best->SetName("PatRec_FOM_best");

      const std::string filepath_FOM_base = output_dir+"/RotationExtraction";

      const std::string filepath_FOM_png  = filepath_FOM_base+".png";
      const std::string filepath_FOM_root = filepath_FOM_base+".root";

      c1->SaveAs(filepath_FOM_png.c_str());

      std::unique_ptr<TFile> o_file(new TFile(filepath_FOM_root.c_str(), "recreate"));
      o_file->cd();
      gr_scan->Write();
      gr_best->Write();
      o_file->Close();

      emit image_path(2, QString::fromStdString(filepath_FOM_png));
    }
    // ---

    const std::string filepath_img_master_copy = output_dir+"/image_master_PatRec.png";
    cv::imwrite(filepath_img_master_copy, img_master_copy);

    emit image_path(1, QString::fromStdString(filepath_img_master_copy));
    emit image_path(3, QString::fromStdString(filepath_img_master_bin));
    emit image_path(4, QString::fromStdString(filepath_img_templa_bin));

    // update line edits in view
    emit reportObjectLocation(1, best_matchLoc.x , best_matchLoc.y, best_theta);

    emit PatRec_exitcode(0);

/*
//    const cv::Rect rect_result = cv::Rect(best_matchLoc, cv::Point(best_matchLoc.x + img_templa_bin.cols, best_matchLoc.y + img_templa_bin.rows));

    //work out match location in field of view
    // the origin of the FOV coordinate system is the top left corner
    //the match loction (centre of the template) is calculated in mm
    //this should be enough for postion correction with moverealtive()

    //matchLoc_x_lab = (best_matchLoc.x +  (img_templa_bin.cols/2) ) * (5.0/img_master.cols); // need to add the current X pos of the lang
    //matchLoc_y_lab = (best_matchLoc.y +  (img_templa_bin.rows/2) ) * (4.0/img_master.rows); // need to add the current Y pos of the lang

*/
}

void MarkerFinderPatRec::PatRec(double& fom, cv::Point& match_loc, const cv::Mat& img_master_bin, const cv::Mat& img_templa_bin, const double angle, const int match_method, const std::string& out_dir) const
{
  // rotated master image
  cv::Mat img_master_bin_rot;

  const cv::Point2f src_center(img_master_bin.cols/2.0F, img_master_bin.rows/2.0F);

  const cv::Mat rot_mat = cv::getRotationMatrix2D(src_center, angle, 1.0);

  const cv::Scalar avgPixelIntensity = cv::mean(img_master_bin);

  warpAffine(img_master_bin, img_master_bin_rot, rot_mat, img_master_bin.size(), cv::INTER_CUBIC, cv::BORDER_CONSTANT, avgPixelIntensity);

  if(out_dir != "")
  {
    const std::string filepath_img_master_bin_rot = out_dir+"/image_master_binary_Rotation_"+std::to_string(angle)+".png";

    cv::imwrite(filepath_img_master_bin_rot, img_master_bin_rot);

    NQLog("MarkerFinderPatRec", NQLog::Spam) << "PatRec"
       << ": saved rotated master-binary image to " << filepath_img_master_bin_rot;
  }
  // -----------

  // matrix with PatRec Figure-Of-Merit values
  cv::Mat result_mat;
  result_mat.create((img_master_bin.rows-img_templa_bin.rows+1), (img_master_bin.cols-img_templa_bin.cols+1), CV_32FC1);

  matchTemplate(img_master_bin_rot, img_templa_bin, result_mat, match_method);

  double minVal, maxVal;
  cv::Point minLoc, maxLoc;

  minMaxLoc(result_mat, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());

  const bool use_minFOM = ((match_method  == CV_TM_SQDIFF) || (match_method == CV_TM_SQDIFF_NORMED));

  if(use_minFOM){ match_loc = minLoc; fom = minVal; }
  else          { match_loc = maxLoc; fom = maxVal; }
  // -----------

  return;
}
