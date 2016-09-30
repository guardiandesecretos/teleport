#ifndef FLEX_CALENDARFAILUREMESSAGE_H
#define FLEX_CALENDARFAILUREMESSAGE_H


#include "CalendarFailureDetails.h"

class CalendarFailureMessage
{
public:
    CalendarFailureDetails details;

    CalendarFailureMessage() { }

    CalendarFailureMessage(CalendarFailureDetails details_);

    static std::string Type() { return "calendar_failure"; }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(details);
    )

    JSON(details);

    DEPENDENCIES();

    HASH160();
};


#endif //FLEX_CALENDARFAILUREMESSAGE_H