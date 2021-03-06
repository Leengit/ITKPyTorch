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
#ifndef itkTorchImage_h
#define itkTorchImage_h

#include <torch/torch.h>
#include <ATen/native/TensorIteratorDynamicCasting.h>
#include "itkSmartPointer.h"
#include "itkImageBase.h"
#include "itkTorchPixelHelper.h"

namespace itk
{
/** \class TorchImage
 *  \brief Templated n-dimensional torch image class.
 *
 * TorchImages are templated over a pixel type (modeling the dependent
 * variables), and a dimension (number of independent variables).  The
 * container for the pixel data is a torch::Tensor.
 *
 * Within the pixel container, torch images are modelled as arrays,
 * defined by a start index and a size.
 *
 * ImageBase, defines the geometry of the torch image in terms of
 * where the torch image sits in physical space, how the torch image
 * is oriented in physical space, the size of a pixel, and the extent
 * of the torch image itself.  ImageBase provides the methods to
 * convert between the index and physical space coordinate frames.
 *
 * Pixels can be accessed directly using the SetPixel() and GetPixel()
 * methods or can be accessed via iterators that define the region of
 * the torch image they traverse.
 *
 * The pixel type may be one of the native types; a Insight-defined
 * class type such as Vector; or a user-defined type. Note that
 * depending on the type of pixel that you use, the process objects
 * (i.e., those filters processing data objects) may not operate on
 * the torch image and/or pixel type. This becomes apparent at
 * compile-time because operator overloading (for the pixel type) is
 * not supported.
 *
 * The data in an torch image is arranged in a 1D array as
 * [][][][slice][row][col] with the column index varying most rapidly.
 * The Index type reverses the order so that with Index[0] = col,
 * Index[1] = row, Index[2] = slice, ...
 *
 * \sa ImageBase
 *
 * \ingroup PyTorch
 *
 */
template< typename TPixel, unsigned int VImageDimension = 2 >
class ITK_TEMPLATE_EXPORT TorchImage : public ImageBase< VImageDimension >
{
public:
  ITK_DISALLOW_COPY_AND_ASSIGN( TorchImage );

  /** Standard class type aliases */
  using Self = TorchImage;
  using Superclass = ImageBase< VImageDimension >;
  using Pointer = SmartPointer< Self >;
  using ConstPointer = SmartPointer< const Self >;
  using ConstWeakPointer = WeakPointer< const Self >;

  /** Method for creation through the object factory. */
  itkNewMacro( Self );

  /** Run-time type information (and related methods). */
  itkTypeMacro( TorchImage, ImageBase );

  /** Pixel type alias support. Used to declare pixel type in filters
   * or other operations. */
  using PixelType = TPixel;

  /** Number of dimensions */
  static constexpr unsigned int ImageDimension = VImageDimension;

  /** Typedef alias for PixelType */
  using ValueType = TPixel;

  /** Internal Pixel representation. Used to maintain a uniform API
   * with Image Adaptors and allow to keep a particular internal
   * representation of data while showing a different external
   * representation. */
  using InternalPixelType = TPixel;

  using IOPixelType = PixelType;

  /** Type of image dimension */
  using ImageDimensionType = typename Superclass::ImageDimensionType;

  /** Index type alias support. An index is used to access pixel values. */
  using IndexType = typename Superclass::IndexType;
  using IndexValueType = typename Superclass::IndexValueType;

  /** Offset type alias support. An offset is used to access pixel values. */
  using OffsetType = typename Superclass::OffsetType;

  /** Size type alias support. A size is used to define region bounds. */
  using SizeType = typename Superclass::SizeType;
  using SizeValueType = typename Superclass::SizeValueType;

  /** Direction type alias support. A matrix of direction cosines. */
  using DirectionType = typename Superclass::DirectionType;

  /** Region type alias support. A region is used to specify a subset of an image.
   */
  using RegionType = typename Superclass::RegionType;

  /** Spacing type alias support.  Spacing holds the size of a pixel.  The
   * spacing is the geometric distance between image samples. */
  using SpacingType = typename Superclass::SpacingType;
  using SpacingValueType = typename Superclass::SpacingValueType;

  /** Origin type alias support.  The origin is the geometric coordinates
   * of the index (0,0). */
  using PointType = typename Superclass::PointType;

  /** Offset type alias (relative position between indices) */
  using OffsetValueType = typename Superclass::OffsetValueType;

  /**
   * example usage:
   * using OutputImageType = typename ImageType::template Rebind< float >::Type;
   *
   * \deprecated Use RebindImageType instead
   */
  template< typename UPixelType, unsigned int NUImageDimension = ImageDimension >
  struct Rebind
  {
    using Type = itk::TorchImage< UPixelType, NUImageDimension >;
  };

  template< typename UPixelType, unsigned int NUImageDimension = ImageDimension >
  using RebindImageType = itk::TorchImage< UPixelType, NUImageDimension >;

  using TorchImagePixelHelper = TorchPixelHelper< PixelType >;
  using DeepScalarType = typename TorchImagePixelHelper::DeepScalarType;
  static constexpr unsigned int PixelDimension = TorchImagePixelHelper::PixelDimension;
  static constexpr unsigned int TorchDimension = ImageDimension + PixelDimension;

  enum DeviceType { itkCPU, itkCUDA };
  enum TensorInitializer { itkEmpty, itkZeros, itkOnes, itkRand, itkRandn };

  /** Select itkCUDA (on device #0) or itkCPU */
  bool SetDevice( DeviceType deviceType );

  /** Select itkCUDA and a device number */
  bool SetDevice( DeviceType deviceType, uint64_t cudaDeviceNumber );

  /** Query current device type and device number */
  void GetDevice( DeviceType &deviceType, uint64_t &cudaDeviceNumber );

  /** Allocate the torch image memory. The size of the torch image
   * must already be set, e.g. by calling SetRegions().  Returns false
   * if allocation to a non-existent GPU fails. */
  void Allocate( TensorInitializer tensorInitializer = itkEmpty );

  /** Restore the data object to its initial state. This means releasing
   * memory. */
  void Initialize() override;

  /** Fill the torch image buffer with a value.  Be sure to call
   * Allocate() first. This version is for pixel types that are
   * simple scalars. */
  template< typename T = void >
  typename std::enable_if< PixelDimension == 0, T >::type
  FillBuffer( const PixelType &value )
    {
    m_Tensor.fill_( value );
    }

  /** Fill the torch image buffer with a value.  Be sure to call
   * Allocate() first. This version is for pixel types that are not
   * simple scalars. */
  template< typename T = void >
  typename std::enable_if< PixelDimension != 0, T >::type
  FillBuffer( const PixelType &value )
    {
    const SizeType &bufferSize = Self::GetBufferedRegion().GetSize();
    std::vector< at::indexing::TensorIndex > TorchIndex;
    FillBufferPart( Self::ImageDimension, bufferSize, TorchIndex, value );
    }

  /** \brief Set a pixel value.
   *
   * Allocate() needs to have been called first -- for efficiency,
   * this function does not check that the torch image has actually
   * been allocated yet. */
  void SetPixel( const IndexType & index, const PixelType & value );

  /** \brief Get a reference to a pixel (e.g. for editing).
   *
   * For efficiency, this function does not check that the
   * torch image has actually been allocated yet. */
  TorchImagePixelHelper GetPixel( const IndexType & index );
  const TorchImagePixelHelper GetPixel( const IndexType & index ) const;

  /** \brief Access a pixel. This version can be an lvalue.
   *
   * For efficiency, this function does not check that the
   * torch image has actually been allocated yet. */
  TorchImagePixelHelper operator[]( const IndexType & index )
    {
    return this->GetPixel( index );
    }

  const TorchImagePixelHelper operator[]( const IndexType & index ) const
    {
    return this->GetPixel( index );
    }

  /** The pointer might be to GPU memory and, if so, could not be
   * dereferenced */
  virtual TPixel *GetBufferPointer();

  /** The pointer might be to GPU memory and, if so, could not be
   * dereferenced */
  virtual const TPixel *GetBufferPointer() const;

  /** Graft the data and information from one image to another. This
   * is a convenience method to setup a second image with all the meta
   * information of another image and use the same pixel
   * container. Note that this method is different than just using two
   * SmartPointers to the same image since separate DataObjects are
   * still maintained. This method is similar to
   * ImageSource::GraftOutput(). The implementation in ImageBase
   * simply calls CopyInformation() and copies the region ivars.
   * The implementation here refers to the superclass' implementation
   * and then copies over the pixel container. */
  virtual void Graft( const Self * data );

  constexpr unsigned int GetNumberOfComponentsPerPixel() const override
    {
    return Self::TorchImagePixelHelper::NumberOfComponents;
    }

protected:
  TorchImage();
  ~TorchImage() override = default;
  void PrintSelf( std::ostream & os, Indent indent ) const override;
  void Graft( const DataObject * data ) override;

  /** Recursively fill part of the full buffer */
  void FillBufferPart( int CurrentDimensions, const SizeType &BufferSize, std::vector< at::indexing::TensorIndex > &TorchIndex, const PixelType &value );

  /** The enum representation of the data type in the underlying torch
   * library. */
  static constexpr at::ScalarType TorchValueType = at::native::cppmap::detail::CPPTypeToScalarType< DeepScalarType >::value();

  /** The first dimension of an image index varies the quickest in the
   * underlying buffer with ITK generally (e.g. class Image) but the
   * first dimension varies slowest with the underlying C++ Torch
   * library, so the index components of this TorchSize are in reverse
   * order compared to the rest of ITK.  Additionally, TorchImage
   * represents a non-scalar pixel type as an additional dimension in
   * the last position, with size equal to the number of components of
   * the non-scalar pixel type, and varying faster than the index
   * dimensions.  The non-scalar pixel representation is recursive in
   * that a non-scalar pixel type A with X components that are a
   * non-scalar pixel type B with Y components would have dimensions
   * of size X and Y beyond the index, with the dimension for B being
   * last and varying the fastest in the underlying buffer. */
  std::vector< int64_t > ComputeTorchSize() const;

  /** Support the ImageBase::Graft methods.
   */
  using Superclass::Graft;

private:
  /** itkCUDA or itkCPU */
  DeviceType m_DeviceType;

  /** Whether tensor has been allocated */
  bool m_Allocated;

  /** Defaults to zero */
  uint64_t m_CudaDeviceNumber;

  /** The torch::Tensor object points to the pixel data and also
   * stores information about size, data type, device, etc. */
  torch::Tensor m_Tensor;
};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkTorchImage.hxx"
#endif

#endif
