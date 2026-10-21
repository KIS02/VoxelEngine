#pragma once



namespace ve
{
    struct MouseDelta
    {
        double x = 0.0;
        double y = 0.0;
    };



    class MouseTracker {
        private:
            double previousX_ = 0.0;
            double previousY_ = 0.0;

            MouseDelta delta_{};

            bool initialized_ = false;


        public:
            void sample( double x, double y );

            void reset();

            MouseDelta delta() const;
    };
}