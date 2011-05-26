#include <retroshare/rsturtle.h>

class TurtleTrafficStatisticsInfoOp: public TurtleTrafficStatisticsInfo
{
	public:
		TurtleTrafficStatisticsInfoOp()
		{
			reset() ;
		}

		void reset()
		{
			unknown_updn_Bps = 0.0f ;
			data_up_Bps = 0.0f ;
			data_dn_Bps = 0.0f ;
			tr_up_Bps = 0.0f ;
			tr_dn_Bps = 0.0f ;
			total_up_Bps = 0.0f ;
			total_dn_Bps = 0.0f ;
		}

		TurtleTrafficStatisticsInfoOp operator*(float f) const
		{
			TurtleTrafficStatisticsInfoOp i(*this) ;

			i.unknown_updn_Bps *= f ;
			i.data_up_Bps *= f ;
			i.data_dn_Bps *= f ;
			i.tr_up_Bps *= f ;
			i.tr_dn_Bps *= f ;
			i.total_up_Bps *= f ;
			i.total_dn_Bps *= f ;

			return i ;
		}
		TurtleTrafficStatisticsInfoOp operator+(const TurtleTrafficStatisticsInfoOp& j) const
		{
			TurtleTrafficStatisticsInfoOp i(*this) ;

			i.unknown_updn_Bps 	+= j.unknown_updn_Bps  ;
			i.data_up_Bps 			+= j.data_up_Bps 		  ;
			i.data_dn_Bps 			+= j.data_dn_Bps 		  ;
			i.tr_up_Bps 			+= j.tr_up_Bps 		  ;
			i.tr_dn_Bps 			+= j.tr_dn_Bps 		  ;
			i.total_up_Bps			+= j.total_up_Bps		  ;
			i.total_dn_Bps			+= j.total_dn_Bps		  ;

			return i ;
		}
};

