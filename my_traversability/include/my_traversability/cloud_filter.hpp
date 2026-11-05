#ifndef MY_TRAVERSABILITY_CLOUD_FILTER_HPP_
#define MY_TRAVERSABILITY_CLOUD_FILTER_HPP_

#include <cmath>
#include <Eigen/Geometry>

namespace my_traversability
{
    class CloudFilter{
        public:
        CloudFilter()=default; //default constructor
        ~CloudFilter()=default; //default distructor

        //function for checking NAN , infinite values for x,y,z
        static inline bool isValid(float x, float y, float z) noexcept
        {
            return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
        }

        //function for checking if points are within the bounding box
        static inline bool isInBoundingBox(float x, float y, float z, float min_x , float max_x , float min_y , float max_y, float min_z, float max_z )noexcept
        {
            return (x>=min_x && x<=max_x) && (y>=min_y && y<=max_y) && (z>=min_z && z<=max_z);
        }

        //Transforming points wrt to Robot chessie 
        static inline void transformPoint(const Eigen::Isometry3f & transform, float in_x , float in_y , float in_z, float & out_x , float & out_y, float & out_z)noexcept
        {
            const Eigen::Vector3f pt(in_x, in_y, in_z);
            const Eigen::Vector3f transformed = transform * pt;
            out_x = transformed.x();
            out_y = transformed.y();
            out_z = transformed.z(); 
        }

        //master function that our perception pipeline will use
        static inline bool filterAndTransform(const Eigen::Isometry3f & transform , float in_x , float in_y , float in_z, float & out_x, float & out_y, float & out_z, float min_z_cutoff, float max_z_cutoff)noexcept
        {
            if (!isValid(in_x, in_y, in_z)){
                return false;
            }
            else{
                transformPoint(transform, in_x, in_y, in_z, out_x, out_y, out_z);
            }
            return (out_z >= min_z_cutoff && out_z <= max_z_cutoff); 
        }

    };

}

#endif  // MY_TRAVERSABILITY_CLOUD_FILTER_HPP_