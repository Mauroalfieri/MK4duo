/**
 * MK4duo Firmware for 3D Printer, Laser and CNC
 *
 * Based on Marlin, Sprinter and grbl
 * Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
 * Copyright (C) 2013 Alberto Cotronei @MagoKimbra
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * bedlevel.h
 *
 * Copyright (C) 2017 Alberto Cotronei @MagoKimbra
 */

#ifndef _BEDLEVEL_H_
#define _BEDLEVEL_H_

#if OLD_ABL
  #define XY_PROBE_FEEDRATE_MM_S bedlevel.xy_probe_feedrate_mm_s
#elif ENABLED(XY_PROBE_SPEED)
  #define XY_PROBE_FEEDRATE_MM_S MMM_TO_MMS(XY_PROBE_SPEED)
#else
  #define XY_PROBE_FEEDRATE_MM_S PLANNER_XY_FEEDRATE()
#endif

#if HAS_LEVELING

  #if ENABLED(AUTO_BED_LEVELING_LINEAR) || ENABLED(AUTO_BED_LEVELING_UBL)
    #include "math/vector_3.h"
    #include "math/least_squares_fit.h"
  #endif
  #if ENABLED(MESH_BED_LEVELING)
    #include "mbl/mesh_bed_leveling.h"
  #elif ENABLED(AUTO_BED_LEVELING_UBL)
    #include "ubl/ubl.h"
  #endif

  #if ENABLED(AUTO_BED_LEVELING_BILINEAR)

    #if ENABLED(ABL_BILINEAR_SUBDIVISION)
      #define ABL_GRID_POINTS_VIRT_X (GRID_MAX_POINTS_X - 1) * (BILINEAR_SUBDIVISIONS) + 1
      #define ABL_GRID_POINTS_VIRT_Y (GRID_MAX_POINTS_Y - 1) * (BILINEAR_SUBDIVISIONS) + 1
      #define ABL_TEMP_POINTS_X (GRID_MAX_POINTS_X + 2)
      #define ABL_TEMP_POINTS_Y (GRID_MAX_POINTS_Y + 2)

      #define ABL_BG_SPACING(A) bedlevel.bilinear_grid_spacing_virt[A]
      #define ABL_BG_FACTOR(A)  bedlevel.bilinear_grid_factor_virt[A]
      #define ABL_BG_POINTS_X   ABL_GRID_POINTS_VIRT_X
      #define ABL_BG_POINTS_Y   ABL_GRID_POINTS_VIRT_Y
      #define ABL_BG_GRID(X,Y)  bedlevel.z_values_virt[X][Y]
    #else
      #define ABL_BG_SPACING(A) bedlevel.bilinear_grid_spacing[A]
      #define ABL_BG_FACTOR(A)  bedlevel.bilinear_grid_factor[A]
      #define ABL_BG_POINTS_X   GRID_MAX_POINTS_X
      #define ABL_BG_POINTS_Y   GRID_MAX_POINTS_Y
      #define ABL_BG_GRID(X,Y)  bedlevel.z_values[X][Y]
    #endif

  #endif

  class Bedlevel {

    public: /** Constructor */

      Bedlevel() {}

    public: /** Public Parameters */

      #if HAS_LEVELING
        static bool leveling_active;          // Flag that bed leveling is enabled
        #if OLD_ABL
          static int xy_probe_feedrate_mm_s;
        #endif
        #if ABL_PLANAR
          static matrix_3x3 matrix; // Transform to compensate for bed level
        #endif
      #endif

      #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
        static float z_fade_height, inverse_z_fade_height;
      #endif

      #if ENABLED(AUTO_BED_LEVELING_BILINEAR)
        static int    bilinear_grid_spacing[2], bilinear_start[2];
        static float  bilinear_grid_factor[2],
                      z_values[GRID_MAX_POINTS_X][GRID_MAX_POINTS_Y];
      #endif

      #if ENABLED(PROBE_MANUALLY)
        static bool g29_in_progress;
      #else
        static const bool g29_in_progress;
      #endif

    private: /** Private Parameters */

      #if ENABLED(AUTO_BED_LEVELING_BILINEAR) && ENABLED(ABL_BILINEAR_SUBDIVISION)
        static float  bilinear_grid_factor_virt[2],
                      z_values_virt[ABL_GRID_POINTS_VIRT_X][ABL_GRID_POINTS_VIRT_Y];
        static int    bilinear_grid_spacing_virt[2];
      #endif

      #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
        static float last_raw_lz;
      #endif

    public: /** Public Function */

      #if PLANNER_LEVELING
        /**
         * Apply leveling to transform a cartesian position
         * as it will be given to the planner and steppers.
         */
        static void apply_leveling(float &lx, float &ly, float &lz);
        static void apply_leveling(float logical[XYZ]) { apply_leveling(logical[X_AXIS], logical[Y_AXIS], logical[Z_AXIS]); }
        static void unapply_leveling(float logical[XYZ]);
      #endif

      #if ENABLED(AUTO_BED_LEVELING_BILINEAR)
        static float bilinear_z_offset(const float logical[XYZ]);
        static void refresh_bed_level();

        /**
         * Fill in the unprobed points (corners of circular print surface)
         * using linear extrapolation, away from the center.
         */
        static void extrapolate_unprobed_bed_level();

        static void print_bilinear_leveling_grid();

        #if ENABLED(ABL_BILINEAR_SUBDIVISION)
          static void print_bilinear_leveling_grid_virt();
          static void virt_interpolate();
        #endif
      #endif

      static bool leveling_is_valid();
      static void set_bed_leveling_enabled(const bool enable=true);
      static void reset();

      #if ENABLED(MESH_BED_LEVELING)
        static void mesh_report();
      #endif

      #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)

        static void set_z_fade_height(const float zfh);

        /**
         * Get the Z leveling fade factor based on the given Z height,
         * re-calculating only when needed.
         *
         *  Returns 1.0 if planner.z_fade_height is 0.0.
         *  Returns 0.0 if Z is past the specified 'Fade Height'.
         */
        inline static float fade_scaling_factor_for_z(const float &lz) {
          static float z_fade_factor = 1.0;
          if (z_fade_height) {
            const float raw_lz = RAW_Z_POSITION(lz);
            if (raw_lz >= z_fade_height) return 0.0;
            if (last_raw_lz != raw_lz) {
              last_raw_lz = raw_lz;
              z_fade_factor = 1.0 - raw_lz * inverse_z_fade_height;
            }
            return z_fade_factor;
          }
          return 1.0;
        }

        FORCE_INLINE static void force_fade_recalc() { last_raw_lz = -999.999; }

        FORCE_INLINE static bool leveling_active_at_z(const float &lz) {
          return !z_fade_height || RAW_Z_POSITION(lz) < z_fade_height;
        }

      #else

        FORCE_INLINE static float fade_scaling_factor_for_z(const float &lz) {
          UNUSED(lz);
          return 1.0;
        }

        FORCE_INLINE static bool leveling_active_at_z(const float &lz) { return true; }

      #endif

    private: /** Private Function */

      #if ENABLED(AUTO_BED_LEVELING_BILINEAR)
        /**
         * Extrapolate a single point from its neighbors
         */
        static void extrapolate_one_point(const uint8_t x, const uint8_t y, const int8_t xdir, const int8_t ydir);

        #if ENABLED(ABL_BILINEAR_SUBDIVISION)
          static float bed_level_virt_coord(const uint8_t x, const uint8_t y);
          static float bed_level_virt_cmr(const float p[4], const uint8_t i, const float t);
          static float bed_level_virt_2cmr(const uint8_t x, const uint8_t y, const float &tx, const float &ty);
        #endif
      #endif

      #if ENABLED(AUTO_BED_LEVELING_BILINEAR) || ENABLED(MESH_BED_LEVELING)

        /**
         * Print calibration results for plotting or manual frame adjustment.
         */
        static void print_2d_array(const uint8_t sx, const uint8_t sy, const uint8_t precision, float (*fn)(const uint8_t, const uint8_t));

      #endif

  };

  extern Bedlevel bedlevel;

#endif // HAS_LEVELING

#endif /* _BEDLEVEL_H_ */
