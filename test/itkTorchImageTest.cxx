/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "itkTorchImage.h"

#include "itkCommand.h"
#include "itkTestingMacros.h"
#include "itkRGBPixel.h"
#include "itkRGBAPixel.h"
#include "itkVector.h"
#include "itkCovariantVector.h"

namespace
{
class ShowProgress : public itk::Command
{
public:
  itkNewMacro( ShowProgress );

  void
  Execute( itk::Object *caller, const itk::EventObject &event ) override
  {
    Execute( ( const itk::Object * )caller, event );
  }

  void
  Execute( const itk::Object *caller, const itk::EventObject &event ) override
  {
    if ( !itk::ProgressEvent().CheckEvent( &event ) )
    {
      return;
    }
    const auto *processObject = dynamic_cast<const itk::ProcessObject *>( caller );
    if ( !processObject )
    {
      return;
    }
    std::cout << " " << processObject->GetProgress();
  }
};
} // namespace

template< typename PixelType, int ImageDimension >
int
itkTorchImageTestByTypeAndDimension(
  const int SizePerDimension,
  const std::string &StructName,
  const PixelType &firstValue,
  const PixelType &secondValue,
  const PixelType &thirdValue )
{
  // Create input image and an observer
  using ImageType = itk::TorchImage< PixelType, ImageDimension >;
  typename ImageType::Pointer image = ImageType::New();
  ShowProgress::Pointer showProgress = ShowProgress::New();
  image->AddObserver( itk::ProgressEvent(), showProgress );

  // Choose itkCUDA as the device type
  typename ImageType::DeviceType MyDeviceType = ImageType::itkCUDA;
  bool response = image->SetDevice( MyDeviceType );
  if ( !response )
    {
    // itkCUDA not available.  Trying itkCPU instead.
    MyDeviceType = ImageType::itkCPU;
    response = image->SetDevice( MyDeviceType );
    }
  itkAssertOrThrowMacro( response, StructName + "::SetDevice failed" );
  typename ImageType::DeviceType deviceType;
  uint64_t cudaDeviceNumber = 0;
  image->GetDevice( deviceType, cudaDeviceNumber );
  itkAssertOrThrowMacro( deviceType == MyDeviceType, StructName + "::GetDevice failed for deviceType" );
  itkAssertOrThrowMacro( cudaDeviceNumber == 0, StructName + "::GetDevice failed for cudaDeviceNumber" );

  typename ImageType::SizeType size;
  size.Fill( SizePerDimension );
  image->SetRegions( size );
  image->Allocate();

  typename ImageType::IndexType location0;
  location0.Fill( 0 );
  location0[0] = 1;             // ( 1, 0, 0, ... )
  typename ImageType::IndexType location1;
  location1.Fill( 1 );
  location1[0] = 0;             // ( 0, 1, 1, ... )
  PixelType pixelValue;

  image->FillBuffer( firstValue );
  pixelValue = image->GetPixel( location0 );
  itkAssertOrThrowMacro( pixelValue == firstValue, StructName + "::FillBuffer failed" );
  pixelValue = image->GetPixel( location1 );
  itkAssertOrThrowMacro( pixelValue == firstValue, StructName + "::FillBuffer failed" );

  image->GetPixel( location0 ) = secondValue;
  pixelValue = image->GetPixel( location0 );
  itkAssertOrThrowMacro( pixelValue == secondValue, StructName + "::GetPixel as lvalue failed" );

  pixelValue = image->GetPixel( location1 );
  itkAssertOrThrowMacro( pixelValue == firstValue, StructName + "::GetPixel has side effect" );

  image->SetPixel( location1, thirdValue );
  pixelValue = image->GetPixel( location1 );
  itkAssertOrThrowMacro( pixelValue == thirdValue, StructName + "::SetPixel failed" );
  pixelValue = image->GetPixel( location0 );
  itkAssertOrThrowMacro( pixelValue == secondValue, StructName + "::SetPixel has side effect" );

  typename ImageType::Pointer image2 = ImageType::New();
  image2->SetRegions( size );
  image2->Graft( image );

  return EXIT_SUCCESS;
}

int itkTorchImageTest( int argc, char *argv[] )
{
  std::cout << "Test compiled " << __DATE__ << " " << __TIME__ << std::endl;

  if ( argc < 2 )
    {
    std::cerr << "Missing parameters." << std::endl;
    std::cerr << "Usage: " << itkNameOfTestExecutableMacro( argv );
    std::cerr << " outputImage";
    std::cerr << std::endl;
    return EXIT_FAILURE;
    }
  // const char * const outputImageFileName = argv[1];

  // Torch supports:
  //   Unsigned integer types: 1, 8 bits.
  //   Signed integer types: 8, 16, 32, 64 bits.
  //   Floating point types: 16, 32, 64 bits
  // though we do not support 16-bit floats.
  {
    using PixelType = bool;
    constexpr int ImageDimension = 6;
    const std::string StructName = "TorchImage<bool, 6>";
    const int SizePerDimension = 3;
    const PixelType firstValue = false;
    const PixelType secondValue = true;
    const PixelType thirdValue = false;
    const int response =
      itkTorchImageTestByTypeAndDimension< PixelType, ImageDimension >( SizePerDimension, StructName, firstValue, secondValue, thirdValue );
    if( response != EXIT_SUCCESS )
      {
      return response;
      }

    // Run ITK_EXERCISE_BASIC_OBJECT_METHODS just once, rather than for every pixel type.
    using ImageType = itk::TorchImage< PixelType, ImageDimension >;
    typename ImageType::Pointer image = ImageType::New();
    ITK_EXERCISE_BASIC_OBJECT_METHODS( image, TorchImage, ImageBase );
  }
  {
    using PixelType = uint8_t;
    constexpr int ImageDimension = 3;
    const std::string StructName = "TorchImage<uint8_t, 3>";
    const int SizePerDimension = 16;
    const PixelType firstValue = 10;
    const PixelType secondValue = 130;
    const PixelType thirdValue = 12;
    const int response =
      itkTorchImageTestByTypeAndDimension< PixelType, ImageDimension >( SizePerDimension, StructName, firstValue, secondValue, thirdValue );
    if( response != EXIT_SUCCESS )
      {
      return response;
      }
  }
  {
    using PixelType = int8_t;
    constexpr int ImageDimension = 4;
    const std::string StructName = "TorchImage<int8_t, 4>";
    const int SizePerDimension = 10;
    const PixelType firstValue = 10;
    const PixelType secondValue = -11;
    const PixelType thirdValue = 12;
    const int response =
      itkTorchImageTestByTypeAndDimension< PixelType, ImageDimension >( SizePerDimension, StructName, firstValue, secondValue, thirdValue );
    if( response != EXIT_SUCCESS )
      {
      return response;
      }
  }
  {
    using PixelType = int16_t;
    constexpr int ImageDimension = 2;
    const std::string StructName = "TorchImage<int16_t, 2>";
    const int SizePerDimension = 256;
    const PixelType firstValue = 32000;
    const PixelType secondValue = -32000;
    const PixelType thirdValue = 5;
    const int response =
      itkTorchImageTestByTypeAndDimension< PixelType, ImageDimension >( SizePerDimension, StructName, firstValue, secondValue, thirdValue );
    if( response != EXIT_SUCCESS )
      {
      return response;
      }
  }
  {
    using PixelType = int32_t;
    constexpr int ImageDimension = 5;
    const std::string StructName = "TorchImage<int32_t, 5>";
    const int SizePerDimension = 8;
    const PixelType firstValue = 2147483000;
    const PixelType secondValue = -2147483000;
    const PixelType thirdValue = 10;
    const int response =
      itkTorchImageTestByTypeAndDimension< PixelType, ImageDimension >( SizePerDimension, StructName, firstValue, secondValue, thirdValue );
    if( response != EXIT_SUCCESS )
      {
      return response;
      }
  }
  {
    using PixelType = int64_t;
    constexpr int ImageDimension = 1;
    const std::string StructName = "TorchImage<int64_t, 1>";
    const int SizePerDimension = 1000;
    const PixelType firstValue = 9223372036854775000LL;
    const PixelType secondValue = -9223372036854775000LL;
    const PixelType thirdValue = 16;
    const int response =
      itkTorchImageTestByTypeAndDimension< PixelType, ImageDimension >( SizePerDimension, StructName, firstValue, secondValue, thirdValue );
    if( response != EXIT_SUCCESS )
      {
      return response;
      }
  }
  {
    using PixelType = float;
    constexpr int ImageDimension = 2;
    const std::string StructName = "TorchImage<float, 2>";
    const int SizePerDimension = 128;
    const PixelType firstValue = 1.1f;
    const PixelType secondValue = -1.2f;
    const PixelType thirdValue = 1.3f;
    const int response =
      itkTorchImageTestByTypeAndDimension< PixelType, ImageDimension >( SizePerDimension, StructName, firstValue, secondValue, thirdValue );
    if( response != EXIT_SUCCESS )
      {
      return response;
      }
  }
  {
    using PixelType = double;
    constexpr int ImageDimension = 3;
    const std::string StructName = "TorchImage<double, 3>";
    const int SizePerDimension = 32;
    const PixelType firstValue = 1.4;
    const PixelType secondValue = -1.5;
    const PixelType thirdValue = 1.6;
    const int response =
      itkTorchImageTestByTypeAndDimension< PixelType, ImageDimension >( SizePerDimension, StructName, firstValue, secondValue, thirdValue );
    if( response != EXIT_SUCCESS )
      {
      return response;
      }
  }

  // We also support various vector pixel types,
  //   RGBPixel, RGBAPixel, Vector, CovariantVector
  // including recursive forms,
  //   Vector<CovariantVector<<RGBPixel<Vector>>>, etc.

  {
    using PixelType = itk::RGBPixel< uint8_t >;
    constexpr int ImageDimension = 3;
    const std::string StructName = "TorchImage<RGBPixel<uint8_t>, 3>";
    const int SizePerDimension = 20;
    const typename PixelType::ValueType firstValue[] = {1, 1, 1};
    const typename PixelType::ValueType secondValue[] = {2, 2, 2};
    const typename PixelType::ValueType thirdValue[] = {2, 3, 1};
    const int response =
      itkTorchImageTestByTypeAndDimension< PixelType, ImageDimension >( SizePerDimension, StructName, firstValue, secondValue, thirdValue );
    if( response != EXIT_SUCCESS )
      {
      return response;
      }
  }
  {
    using PixelType = itk::RGBAPixel< uint8_t >;
    constexpr int ImageDimension = 2;
    const std::string StructName = "TorchImage<RGBAPixel<uint8_t>, 2>";
    const int SizePerDimension = 30;
    const typename PixelType::ValueType firstValue[] = {1, 1, 1, 255};
    const typename PixelType::ValueType secondValue[] = {2, 2, 2, 128};
    const typename PixelType::ValueType thirdValue[] = {2, 3, 1, 64};
    const int response =
      itkTorchImageTestByTypeAndDimension< PixelType, ImageDimension >( SizePerDimension, StructName, firstValue, secondValue, thirdValue );
    if( response != EXIT_SUCCESS )
      {
      return response;
      }
  }
  {
    constexpr int VectorDimension = 2;
    using PixelType = itk::Vector< int16_t, VectorDimension >;
    constexpr int ImageDimension = 2;
    const std::string StructName = "TorchImage<Vector<int16_t, 2>, 2>";
    const int SizePerDimension = 250;
    const typename PixelType::ValueType firstValue[VectorDimension] = {1, 2};
    const typename PixelType::ValueType secondValue[VectorDimension] = {-100, 32000};
    const typename PixelType::ValueType thirdValue[VectorDimension] = {100, -32000};
    const int response =
      itkTorchImageTestByTypeAndDimension< PixelType, ImageDimension >( SizePerDimension, StructName, firstValue, secondValue, thirdValue );
    if( response != EXIT_SUCCESS )
      {
      return response;
      }
  }

  {
    constexpr int VectorDimension = 3;
    using PixelType = itk::Vector< int16_t, VectorDimension >;
    constexpr int ImageDimension = 4;
    const std::string StructName = "TorchImage<Vector<int16_t, 3>, 4>";
    const int SizePerDimension = 12;
    const typename PixelType::ValueType firstValue[VectorDimension] = {1, 2, 3};
    const typename PixelType::ValueType secondValue[VectorDimension] = {-310, 3100, -31000};
    const typename PixelType::ValueType thirdValue[VectorDimension] = {310, -3100, 31000};
    const int response =
      itkTorchImageTestByTypeAndDimension< PixelType, ImageDimension >( SizePerDimension, StructName, firstValue, secondValue, thirdValue );
    if( response != EXIT_SUCCESS )
      {
      return response;
      }
  }
  {
    constexpr int VectorDimension = 4;
    using PixelType = itk::CovariantVector< int16_t, VectorDimension >;
    constexpr int ImageDimension = 5;
    const std::string StructName = "TorchImage<CovariantVector<int16_t, 4>, 5>";
    const int SizePerDimension = 4;
    const typename PixelType::ValueType firstValue[VectorDimension] = {1, 2, 3, 4};
    const typename PixelType::ValueType secondValue[VectorDimension] = {-310, 3100, -31000, 31};
    const typename PixelType::ValueType thirdValue[VectorDimension] = {310, -3100, 31000, 31};
    const int response =
      itkTorchImageTestByTypeAndDimension< PixelType, ImageDimension >( SizePerDimension, StructName, firstValue, secondValue, thirdValue );
    if( response != EXIT_SUCCESS )
      {
      return response;
      }
  }
  {
    constexpr int VectorDimension1 = 2;
    constexpr int VectorDimension2 = 3;
    using PixelType = itk::Vector< itk::Vector< itk::RGBPixel< uint8_t >, VectorDimension1 >, VectorDimension2 >;
    constexpr int ImageDimension = 4;
    const std::string StructName = "TorchImage< Vector< Vector< RGBAPixel< uint8_t >, 2 >, 3 >, 4 >";
    const int SizePerDimension = 3;
    const uint8_t firstValue0[] = {1, 1, 1};
    const uint8_t secondValue0[] = {4, 64, 255};
    const uint8_t thirdValue0[] = {0, 128, 1};
    const itk::RGBPixel< uint8_t > firstValue1[VectorDimension1] = { firstValue0, secondValue0 };
    const itk::RGBPixel< uint8_t > secondValue1[VectorDimension1] = { secondValue0, thirdValue0 };
    const itk::RGBPixel< uint8_t > thirdValue1[VectorDimension1] = { thirdValue0, firstValue0 };
    const itk::Vector< itk::RGBPixel< uint8_t >, VectorDimension1 > firstValue[VectorDimension2] =
      { firstValue1, secondValue1, firstValue1 };
    const itk::Vector< itk::RGBPixel< uint8_t >, VectorDimension1 > secondValue[VectorDimension2] =
      { thirdValue1, thirdValue1, thirdValue1 };
    const itk::Vector< itk::RGBPixel< uint8_t >, VectorDimension1 > thirdValue[VectorDimension2] =
      { secondValue1, firstValue1, secondValue1 };
    const int response =
      itkTorchImageTestByTypeAndDimension< PixelType, ImageDimension >( SizePerDimension, StructName, firstValue, secondValue, thirdValue );
    if( response != EXIT_SUCCESS )
      {
      return response;
      }
  }

  std::cout << "Test finished." << std::endl;
  return EXIT_SUCCESS;
}
