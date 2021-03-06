// ========================================================================== //
// This file is part of Sara, a basic set of libraries in C++ for computer
// vision.
//
// Copyright (C) 2013-2016 David Ok <david.ok8@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
// ========================================================================== //

//! @file

#pragma once

#include "DerivedQObjects/GraphicsApplicationImpl.hpp"


namespace DO { namespace Sara {

  /*!
    \addtogroup GraphicsInternal

    @{
   */

  //! @{
  //! Convenience functions.
  inline GraphicsApplication::Impl * gui_app()
  {
    return qobject_cast<GraphicsApplication::Impl *>(qApp);
  }

  inline UserThread& get_user_thread()
  {
    return gui_app()->m_userThread;
  }

  inline bool active_window_is_visible()
  {
    return gui_app()->activeWindowIsVisible();
  }

  inline QImage as_QImage(const ImageView<Rgb8>& image)
  {
    return QImage{reinterpret_cast<const unsigned char *>(image.data()),
                  image.width(), image.height(), image.width() * 3,
                  QImage::Format_RGB888};
  }

  inline QImage as_QImage(ImageView<Rgb8>& image)
  {
    return QImage{reinterpret_cast<unsigned char *>(image.data()),
                  image.width(), image.height(), image.width() * 3,
                  QImage::Format_RGB888};
  }

  inline QColor to_QColor(const Color3ub& c)
  {
    return QColor{c[0], c[1], c[2]};
  }
  //! @}

  //! @}

} /* namespace Sara */
} /* namespace DO */
