/************************************************************************/
/*                                                                      */
/*               Copyright 2001-2002 by Gunnar Kedenburg                */
/*       Cognitive Systems Group, University of Hamburg, Germany        */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    ( Version 1.2.0, Aug 07 2003 )                                    */
/*    You may use, modify, and distribute this software according       */
/*    to the terms stated in the LICENSE file included in               */
/*    the VIGRA distribution.                                           */
/*                                                                      */
/*    The VIGRA Website is                                              */
/*        http://kogs-www.informatik.uni-hamburg.de/~koethe/vigra/      */
/*    Please direct questions, bug reports, and contributions to        */
/*        koethe@informatik.uni-hamburg.de                              */
/*                                                                      */
/*  THIS SOFTWARE IS PROVIDED AS IS AND WITHOUT ANY EXPRESS OR          */
/*  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED      */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. */
/*                                                                      */
/************************************************************************/

#include <config.h>

#ifdef HasPNG

#include <stdexcept>
#include <iostream>
#include "void_vector.hxx"
#include "auto_file.hxx"
#include "png.hxx"
#include "error.hxx"

extern "C"
{
#include <png.h>
}

#if PNG_LIBPNG_VER < 10201
#error "please update your libpng to at least 1.2.1"
#endif

// TODO: per-scanline writing

namespace vigra {

    CodecDesc PngCodecFactory::getCodecDesc() const
    {
        CodecDesc desc;
	
        // init file type
        desc.fileType = "PNG";

        // init pixel types
        desc.pixelTypes.resize(2);
        desc.pixelTypes[0] = "UINT8";
        desc.pixelTypes[1] = "UINT16";

        // init compression types
        desc.compressionTypes.resize(1);
        desc.compressionTypes[0] = "LOSSLESS";

        // init magic strings
        desc.magicStrings.resize(1);
        desc.magicStrings[0].resize(4);
        desc.magicStrings[0][0] = '\x89';
        desc.magicStrings[0][1] = 'P';
        desc.magicStrings[0][2] = 'N';
        desc.magicStrings[0][3] = 'G';

        // init file extensions
        desc.fileExtensions.resize(1);
        desc.fileExtensions[0] = "png";

        return desc;
    }

    std::auto_ptr<Decoder> PngCodecFactory::getDecoder() const
    {
        return std::auto_ptr<Decoder>( new PngDecoder() );
    }

    std::auto_ptr<Encoder> PngCodecFactory::getEncoder() const
    {
        return std::auto_ptr<Encoder>( new PngEncoder() );
    }

    namespace {
        std::string png_error_message;
    }

    // called on fatal errors
    static void PngError( png_structp png_ptr, png_const_charp error_msg )
    {
        png_error_message = std::string(error_msg);
        longjmp( png_ptr->jmpbuf, 1 );
    }

    // called on non-fatal errors
    static void PngWarning( png_structp png_ptr, png_const_charp warning_msg )
    {
        std::cerr << warning_msg << std::endl;
    }

    struct PngDecoderImpl
    {
        // data source
        auto_file file;

        // data container
        void_vector_base bands;

        // this is where libpng stores its state
        png_structp png;
        png_infop info;

        // image header fields
        png_uint_32 width, height, components;
	png_uint_32 extra_components;
	vigra::Diff2D position;
        int bit_depth, color_type;

        // scanline counter
        int scanline;

	float x_resolution, y_resolution;

        // number of passes needed during reading each scanline
        int interlace_method, n_interlace_passes;

        // number of channels in png (or what libpng get_channels returns)
        int n_channels;

        // size of one row
        int rowsize;
        unsigned char * row_data;

        // ctor, dtor
        PngDecoderImpl( const std::string & filename );
        ~PngDecoderImpl();

        // methods
        void init();
        void nextScanline();
    };

    PngDecoderImpl::PngDecoderImpl( const std::string & filename )
#ifdef WIN32
        : file( filename.c_str(), "rb" ),
#else
        : file( filename.c_str(), "r" ),
#endif
          bands(0), scanline(-1), x_resolution(0), y_resolution(0),
          n_interlace_passes(0), n_channels(0), row_data(0)
    {
        png_error_message = "";
        // check if the file is a png file
        const unsigned int sig_size = 8;
        png_byte sig[sig_size];
        std::fread( sig, sig_size, 1, file.get() );
        const int no_png = png_sig_cmp( sig, 0, sig_size );
        vigra_precondition( !no_png, "given file is not a png file.");

        // create png read struct with user defined handlers
        png = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL,
                                      &PngError, &PngWarning );
        vigra_postcondition( png != 0, "could not create the read struct." );

        // create info struct
        if (setjmp(png->jmpbuf)) {
            png_destroy_read_struct( &png, &info, NULL );
            vigra_postcondition( false, png_error_message.insert(0, "error in png_create_info_struct(): ").c_str() );
        }
        info = png_create_info_struct(png);
        vigra_postcondition( info != 0, "could not create the info struct." );

        // init png i/o
        if (setjmp(png->jmpbuf)) {
            png_destroy_read_struct( &png, &info, NULL );
            vigra_postcondition( false, png_error_message.insert(0, "error in png_init_io(): ").c_str() );
        }
        png_init_io( png, file.get() );

        // specify that the signature was already read
        if (setjmp(png->jmpbuf)) {
            png_destroy_read_struct( &png, &info, NULL );
            vigra_postcondition( false, png_error_message.insert(0, "error in png_set_sig_bytes(): ").c_str() );
        }
        png_set_sig_bytes( png, sig_size );

    }

    PngDecoderImpl::~PngDecoderImpl()
    {
        png_destroy_read_struct( &png, &info, NULL );
        if (row_data) delete[] row_data;
    }

    void PngDecoderImpl::init()
    {
        // read all chunks up to the image data
        if (setjmp(png->jmpbuf))
            vigra_postcondition( false, png_error_message.insert(0, "error in png_read_info(): ").c_str() );
        png_read_info( png, info );

        // pull over the header fields
        int compression_method, filter_method;
        if (setjmp(png->jmpbuf))
            vigra_postcondition( false, png_error_message.insert(0, "error in png_get_IHDR(): ").c_str() );
        png_get_IHDR( png, info, &width, &height, &bit_depth, &color_type,
                      &interlace_method, &compression_method, &filter_method );

        // transform palette to rgb
        if ( color_type == PNG_COLOR_TYPE_PALETTE) {
            if (setjmp(png->jmpbuf))
                vigra_postcondition( false, png_error_message.insert(0, "error in png_palette_to_rgb(): ").c_str() );
            png_set_palette_to_rgb(png);
            color_type = PNG_COLOR_TYPE_RGB;
            bit_depth = 8;
        }

        // expand gray values to at least one byte size
        if ( color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8 ) {
            if (setjmp(png->jmpbuf))
                vigra_postcondition( false,png_error_message.insert(0, "error in png_set_gray_1_2_4_to_8(): ").c_str());
            png_set_gray_1_2_4_to_8(png);
            bit_depth = 8;
        }

        // swap bytes if we are on a little endian system.
        // 16 bit png's are stored in big endian byte order
        if (bit_depth == 16) {
            unsigned char swapTest[2] = { 1, 0 };
            if( *(short *) swapTest == 1 ) {
                // little endian, swap
                // expand gray values to at least one byte size
                if (setjmp(png->jmpbuf))
                    vigra_postcondition( false,png_error_message.insert(0, "error in png_set_swap(): ").c_str());
                png_set_swap(png);
            }
        }

	// dangelo: keep the alpha channel
#if 0
        // strip alpha channel
        if ( color_type & PNG_COLOR_MASK_ALPHA ) {
            if (setjmp(png->jmpbuf))
                vigra_postcondition( false, png_error_message.insert(0, "error in png_set_strip_alpha(): ").c_str() );
            png_set_strip_alpha(png);
            color_type ^= PNG_COLOR_MASK_ALPHA;
        }
#endif

        // find out the number of components
        switch (color_type) {
        case PNG_COLOR_TYPE_GRAY:
            components = 1;
	    extra_components = 0;
            break;
	case PNG_COLOR_TYPE_GRAY_ALPHA:
	    components = 2;
	    extra_components = 1;
	    break;
        case PNG_COLOR_TYPE_RGB:
            components = 3;
	    extra_components = 0;
            break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
	    components = 4;
	    extra_components = 1;
	    break;
        default:
            vigra_fail( "internal error: illegal color type." );
        }

	// read resolution
	x_resolution = png_get_x_pixels_per_meter( png, info ) / 254.0;
	y_resolution = png_get_y_pixels_per_meter( png, info ) / 254.0;

	// read offset
	position.x = png_get_x_offset_pixels( png, info );
	position.y = png_get_y_offset_pixels( png, info );

#if 0
        // gamma correction changes the pixels, this is unwanted.

        // image gamma
        double image_gamma = 0.45455;
        if ( png_get_valid( png, info, PNG_INFO_gAMA ) ) {
            if (setjmp(png->jmpbuf))
                vigra_postcondition( false, png_error_message.insert(0, "error in png_get_gAMA(): ").c_str() );
            png_get_gAMA( png, info, &image_gamma );
        }

        // screen gamma
        double screen_gamma = 2.2;

        // set gamma correction
        if (setjmp(png->jmpbuf))
            vigra_postcondition( false, png_error_message.insert(0, "error in png_set_gamma(): ").c_str() );
        png_set_gamma( png, screen_gamma, image_gamma );
#endif

        // interlace handling, get number of read passes needed
        if (setjmp(png->jmpbuf))
            vigra_postcondition( false,png_error_message.insert(0, "error in png_set_interlace_handling(): ").c_str());
        n_interlace_passes = png_set_interlace_handling(png);


        // update png library state to reflect any changes that were made
        if (setjmp(png->jmpbuf))
            vigra_postcondition( false, png_error_message.insert(0, "error in png_read_update_info(): ").c_str() );
        png_read_update_info( png, info );

        if (setjmp(png->jmpbuf))
            vigra_postcondition( false,png_error_message.insert(0, "error in png_get_channels(): ").c_str());
        n_channels = png_get_channels(png, info);

        if (setjmp(png->jmpbuf))
            vigra_postcondition( false,png_error_message.insert(0, "error in png_get_rowbytes(): ").c_str());
        rowsize = png_get_rowbytes(png, info);

        // allocate data buffers
        row_data = new unsigned char[rowsize];
    }

    void PngDecoderImpl::nextScanline()
    {
        for (int i=0; i < n_interlace_passes; i++) {
            if (setjmp(png->jmpbuf))
                vigra_postcondition( false,png_error_message.insert(0, "error in png_read_row(): ").c_str());
            png_read_row(png, row_data, NULL);
        }
    }

    void PngDecoder::init( const std::string & filename )
    {
        pimpl = new PngDecoderImpl(filename);
        pimpl->init();
    }

    PngDecoder::~PngDecoder()
    {
        delete pimpl;
    }

    std::string PngDecoder::getFileType() const
    {
        return "PNG";
    }

    unsigned int PngDecoder::getWidth() const
    {
        return pimpl->width;
    }

    unsigned int PngDecoder::getHeight() const
    {
        return pimpl->height;
    }

    unsigned int PngDecoder::getNumBands() const
    {
        return pimpl->components;
    }

    unsigned int PngDecoder::getNumExtraBands() const
    {
        return pimpl->extra_components;
    }

    float PngDecoder::getXResolution() const
    {
	return pimpl->x_resolution;
    }

    float PngDecoder::getYResolution() const
    {
	return pimpl->y_resolution;
    }

    vigra::Diff2D PngDecoder::getPosition() const
    {
	return pimpl->position;
    }

    std::string PngDecoder::getPixelType() const
    {
        switch (pimpl->bit_depth) {
        case 8:
            return "UINT8";
        case 16:
            return "UINT16";
        default:
            vigra_fail( "internal error: illegal pixel type." );
        }
        return "";
    }

    unsigned int PngDecoder::getOffset() const
    {
        return pimpl->components;
    }

    const void * PngDecoder::currentScanlineOfBand( unsigned int band ) const
    {
        switch (pimpl->bit_depth) {
        case 8:
            {
                return pimpl->row_data + band;
            }
        case 16:
            {
                return pimpl->row_data + 2*band;
            }
        default:
            vigra_fail( "internal error: illegal bit depth." );
        }
        return 0;
    }

    void PngDecoder::nextScanline()
    {
        pimpl->nextScanline();
    }

    void PngDecoder::close() {}

    void PngDecoder::abort() {}

    struct PngEncoderImpl
    {
        // data sink
        auto_file file;

        // data container
        void_vector_base bands;

        // this is where libpng stores its state
        png_structp png;
        png_infop info;

        // image header fields
        png_uint_32 width, height, components;
	png_uint_32 extra_components;
        int bit_depth, color_type;


        // scanline counter
        int scanline;

        // state
        bool finalized;

        // image layer position
	vigra::Diff2D position;

        // resolution
	float x_resolution, y_resolution;

        // ctor, dtor
        PngEncoderImpl( const std::string & filename );
        ~PngEncoderImpl();

        // methods
        void finalize();
        void write();
    };

    PngEncoderImpl::PngEncoderImpl( const std::string & filename )
#ifdef WIN32
        : file( filename.c_str(), "wb" ),
#else
        : file( filename.c_str(), "w" ),
#endif
          bands(0),
          scanline(0), finalized(false),
	  x_resolution(0), y_resolution(0)
    {
        png_error_message = "";
        // create png struct with user defined handlers
        png = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL,
                                       &PngError, &PngWarning );
        vigra_postcondition( png != 0, "could not create the write struct." );

        // create info struct
        if (setjmp(png->jmpbuf)) {
            png_destroy_write_struct( &png, &info );
            vigra_postcondition( false, png_error_message.insert(0, "error in png_info_struct(): ").c_str() );
        }
        info = png_create_info_struct(png);
        if ( !info ) {
            png_destroy_write_struct( &png, &info );
            vigra_postcondition( false, png_error_message.insert(0, "could not create the info struct.: ").c_str() );
        }

        // init png i/o
        if (setjmp(png->jmpbuf)) {
            png_destroy_write_struct( &png, &info );
            vigra_postcondition( false, png_error_message.insert(0, "error in png_init_io(): ").c_str() );
        }
        png_init_io( png, file.get() );
    }

    PngEncoderImpl::~PngEncoderImpl()
    {
        png_destroy_write_struct( &png, &info );
    }

    void PngEncoderImpl::finalize()
    {
        // write the IHDR
        if (setjmp(png->jmpbuf))
            vigra_postcondition( false, png_error_message.insert(0, "error in png_set_IHDR(): ").c_str() );
        png_set_IHDR( png, info, width, height, bit_depth, color_type,
                      PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                      PNG_FILTER_TYPE_DEFAULT );

	// set resolution
	if (x_resolution > 0 && y_resolution > 0) {
	    if (setjmp(png->jmpbuf))
		vigra_postcondition( false, png_error_message.insert(0, "error in png_set_pHYs(): ").c_str() );
	    png_set_pHYs(png, info, (png_uint_32) (x_resolution * 254 + 0.5),
			 (png_uint_32) (y_resolution * 254 + 0.5),
			 PNG_RESOLUTION_METER);
	}

	// set offset
	if (position.x > 0 && position.y > 0) {
	    if (setjmp(png->jmpbuf))
		vigra_postcondition( false, png_error_message.insert(0, "error in png_set_oFFs(): ").c_str() );
	    png_set_oFFs(png, info, position.x, position.y, PNG_OFFSET_PIXEL);
	}

        // write the info struct
        if (setjmp(png->jmpbuf))
            vigra_postcondition( false, png_error_message.insert(0, "error in png_write_info(): ").c_str() );
        png_write_info( png, info );

        // prepare the bands
        bands.resize( ( bit_depth >> 3 ) * width * components * height );

        // enter finalized state
        finalized = true;
    }

    void PngEncoderImpl::write()
    {
        // prepare row pointers
        png_uint_32 row_stride = ( bit_depth >> 3 ) * width * components;
        void_vector<png_byte *>  row_pointers(height);
        typedef void_vector<png_byte> vector_type;
        vector_type & cbands = static_cast< vector_type & >(bands);
        png_byte * mover = cbands.data();
        for( png_uint_32 i = 0; i < height; ++i ) {
            row_pointers[i] = mover;
            mover += row_stride;
        }
        // write the whole image
        if (setjmp(png->jmpbuf))
            vigra_postcondition( false, png_error_message.insert(0, "error in png_write_image(): ").c_str() );
        png_write_image( png, row_pointers.begin() );
        if (setjmp(png->jmpbuf))
            vigra_postcondition( false, png_error_message.insert(0, "error in png_write_end(): ").c_str() );
        png_write_end(png, info);
    }

    void PngEncoder::init( const std::string & filename )
    {
        pimpl = new PngEncoderImpl(filename);
    }

    PngEncoder::~PngEncoder()
    {
        delete pimpl;
    }

    std::string PngEncoder::getFileType() const
    {
        return "PNG";
    }

    void PngEncoder::setWidth( unsigned int width )
    {
        VIGRA_IMPEX2_FINALIZED(pimpl->finalized);
        pimpl->width = width;
    }

    void PngEncoder::setHeight( unsigned int height )
    {
        VIGRA_IMPEX2_FINALIZED(pimpl->finalized);
        pimpl->height = height;
    }

    void PngEncoder::setNumBands( unsigned int bands )
    {
        VIGRA_IMPEX2_FINALIZED(pimpl->finalized);
        if ( bands == 1 )
            pimpl->color_type = PNG_COLOR_TYPE_GRAY;
        else if ( bands == 2 )
            pimpl->color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
        else if ( bands == 3 )
            pimpl->color_type = PNG_COLOR_TYPE_RGB;
        else if ( bands == 4 )
            pimpl->color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        else
            vigra_fail( "internal error: number of components not supported." );
        pimpl->components = bands;
    }



    void PngEncoder::setCompressionType( const std::string & comp,
                                         int quality )
    {
        // nothing is settable => do nothing
    }

    void PngEncoder::setPosition( const vigra::Diff2D & pos )
    {
        VIGRA_IMPEX2_FINALIZED(pimpl->finalized);
	pimpl->position = pos;
    }

    void PngEncoder::setXResolution( float xres )
    {
        VIGRA_IMPEX2_FINALIZED(pimpl->finalized);
	pimpl->x_resolution = xres;
    }

    void PngEncoder::setYResolution( float yres )
    {
        VIGRA_IMPEX2_FINALIZED(pimpl->finalized);
	pimpl->y_resolution = yres;
    }

    void PngEncoder::setPixelType( const std::string & pixelType )
    {
        VIGRA_IMPEX2_FINALIZED(pimpl->finalized);
        if ( pixelType == "UINT8" )
            pimpl->bit_depth = 8;
        else if ( pixelType == "UINT16" )
            pimpl->bit_depth = 16;
        else
            vigra_fail( "internal error: pixeltype not supported." );
    }

    unsigned int PngEncoder::getOffset() const
    {
        return pimpl->components;
    }

    void PngEncoder::finalizeSettings()
    {
        VIGRA_IMPEX2_FINALIZED(pimpl->finalized);
        pimpl->finalize();
    }

    void * PngEncoder::currentScanlineOfBand( unsigned int band )
    {
        const unsigned int index = pimpl->width * pimpl->components
            * pimpl->scanline + band;
        switch (pimpl->bit_depth) {
        case 8:
            {
                typedef void_vector< unsigned char > bands_type;
                bands_type & bands
                    = static_cast< bands_type & >(pimpl->bands);
                return bands.data() + index;
            }
        case 16:
            {
                typedef void_vector<short> bands_type;
                bands_type & bands
                    = static_cast< bands_type & >(pimpl->bands);
                return bands.data() + index;
            }
        default:
            vigra_fail( "internal error: illegal bit depth." );
        }
        return 0;
    }

    void PngEncoder::nextScanline()
    {
        ++(pimpl->scanline);
    }

    void PngEncoder::close()
    {
        pimpl->write();
    }

    void PngEncoder::abort() {}
}

#endif // HasPNG
