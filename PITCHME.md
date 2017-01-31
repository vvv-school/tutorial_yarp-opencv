#HSLIDE

### Robot vision tutorial with OpenCV
### <span style="color:#e49436">Part Two</span>
---
#### YARP OPENCV

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
### Read an Image from a stream

#VSLIDE
### Read an Image from a stream

######<div style="text-align: left;">IDL Services </div>
```c++
/**
 * Load the two required images.
 * @param mainImage name of the image to be loaded.
 * @return true/false on success/failure.
 */
bool load(1:string image);
```
---

######<div style="text-align: left;">Code </div>
```c++
yarp::os::ResourceFinder rf;
rf.setVerbose();
rf.setDefaultContext(this->rf->getContext().c_str());

std::string imageStr = rf.findFile(image.c_str());

cv::Mat inputImage;
inputImage = cv::imread(imageStr, CV_LOAD_IMAGE_COLOR);
```
#HSLIDE
### Stream the image onto a YARP port

#VSLIDE
### Stream the image onto a YARP port

```c++
yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> >    imageOutPort;

```
---
```c++
imageOutPort.open(("/"+getName("/image:o")).c_str());
```
---
```c++
imageOutPort.close();
```
---
```c++
cvtColor(out_image, out_image, CV_BGR2RGB);

IplImage yarpImg = out_image;
outImg.resize(yarpImg.width, yarpImg.height);
cvCopy( &yarpImg, (IplImage *) outImg.getIplImage());

imageOutPort.write();
```

#HSLIDE
### Run the template tracker algorithm

#VSLIDE
### Run the template tracker algorithm
######<div style="text-align: left;">IDL Services </div>
```c++
/**
 * use template matching on image with desired template and
 * desired method
 * @param template name of the image to be loaded.
 * @param name of method: 0=SQDIFF, 1=SQDIFF NORMED,
 * 2=TM CCORR, 3=TM CCORR NORMED, 4=TM COEFF, 5=TM COEFF NORMED
 * @return Bottle containing the 2D position.
 */
Bottle templateMatch(1:string image, 2:i32 method);
```
######<div style="text-align: left;">template methods </div>
```c++
//Use the OpenCV function matchTemplate to search for matches between an
//image patch and an input image
void matchTemplate(InputArray image, InputArray templ, OutputArray result, int method);
//Use the OpenCV function minMaxLoc to find the maximum and minimum values
//(as well as their positions) in a given array.
void minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat() );
```

#HSLIDE
The End :)
