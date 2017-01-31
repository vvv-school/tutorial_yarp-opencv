#HSLIDE

### Robot vision tutorial with OpenCV
### <span style="color:#e49436">Part Two</span>
---
#### YARP & OPENCV

#HSLIDE
### Goals of this Tutorial
 - Track something <span style="color:#e49436">round</span> and <span style="color:#e49436">red</span> :-)
 - integrating <span style="color:#e49436">YARP</span> with <span style="color:#e49436">OpenCV</span> while getting
 <span style="color:#e49436">live</span> image <span style="color:#e49436">streams</span>.
 - yarp::os::<span style="color:#e49436">RFModule</span> with port <span style="color:#e49436">Callbacks</span>
 - <span style="color:#e49436">Thrift</span> services
 - performing simple <span style="color:#e49436">image processing</span> operations

#VSLIDE
### Let's plan what to do...
 - <span style="color:#e49436">Receive</span> a stream of images from a port
 - Use some <span style="color:#e49436">image processing</span> techniques to make things easier.
 - Display it: <span style="color:#e49436">stream</span> it through a <span style="color:#e49436">yarp port</span> to a <span style="color:#e49436">yarpviewer</span>.
 - Modify the streamed image to <span style="color:#e49436">display</span> the <span style="color:#e49436">location</span> of the red and round object.

#HSLIDE
### Read an Image from a stream using port callback

#VSLIDE
### Read an Image from a stream using port callback

<!--######<div style="text-align: left;">Code </div> -->
```c++
class Module : public yarp::os::RFModule, public yarpOpencv_IDL
{
    ...
```
```c++
class Processing : public yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> >
{
bool open(){
    this->useCallback();
    BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> >::open( "/" + moduleName + "/image:i" );
    ...
}
void interrupt(){
    BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> >::interrupt();
}
void close()(){
    BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> >::close();
}
void onRead( yarp::sig::ImageOf<yarp::sig::PixelRgb> &img ){
    cv::Mat in_cv = cv::cvarrToMat((IplImage *)img.getIplImage());
}
```
#HSLIDE
### Some Image Processing techniques

#VSLIDE
### Some Image Processing techniques
######<div style="text-align: left;">Spatial Filters </div>
```c++
cv::GaussianBlur(redBallOnly, redBallOnly, cv::Size(gausian_size, gausian_size), 2, 2);

```
######<div style="text-align: left;">Morphology </div>
```c++
cv::dilate(redBallOnly, redBallOnly, cv::Mat(), cv::Point(-1,-1), dilate_niter, cv::BORDER_CONSTANT, cv::morphologyDefaultBorderValue());
```
---
```c++
cv::erode(redBallOnly, redBallOnly, cv::Mat(), cv::Point(-1,-1), erode_niter, cv::BORDER_CONSTANT, cv::morphologyDefaultBorderValue());
```
######<div style="text-align: left;">Detect Circles </div>
```c++
cv::HoughCircles(redBallOnly, circles, CV_HOUGH_GRADIENT, 1, redBallOnly.rows / 8, HIGH_THRESHOLD, HOUGH_MIN_VOTES, HOUGH_MIN_RADIUS, HOUGH_MAX_RADIUS);
```

#HSLIDE
### Draw and display results

#VSLIDE
### Draw and display results
```c++
for (size_t i = 0; i < circles.size(); i++)
{
    cv::Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
    int radius = cvRound(circles[i][2]);
    // circle center
    circle(in_cv, center, 3, cv::Scalar(0, 255, 0), -1, 8, 0);
    // circle outline
    circle(in_cv, center, radius, cv::Scalar(0, 0, 255), 3, 8, 0);

    yarp::os::Bottle &t=outTargets.addList();
    t.addDouble(center.x);
    t.addDouble(center.y);
}
```

#HSLIDE
The End :)
