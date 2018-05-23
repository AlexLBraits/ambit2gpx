#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <libambit.h>

/////////////////////////////////////////////////////////////
char time_string[32] = {0};
char activity_name[32] = {0};
double latitude = 0;
double longitude = 0;
double elevation = 0;
double distance = 0;
double speed = 0;
double vertical_speed = 0;
double sea_level_pressure = 0;
uint8_t hr = 0;
double energy = 0;
double temperature = 0;
/////////////////////////////////////////////////////////////
char* time_stamp = 0;

void print_header(ambit_log_header_t *log_header);
void print_footer();
int log_skip_cb(void *ambit_object, ambit_log_header_t *log_header);
void log_data_cb(void *object, ambit_log_entry_t *log_entry);

int main(int argc, char *argv[])
{
    ambit_object_t *ambit_object;
    ambit_device_info_t info;
    ambit_device_status_t status;
    ambit_personal_settings_t settings;

    if (argc > 1)
    {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "/?") == 0)
        {
            fprintf(stderr, 
                "\n"
                "%s - Console program, that read Activity data from Suunto Ambit over USB and write it as gpx-file. Replacement for Moveslink2, that not work without connection to Movescount.\n\n"
                "Usage:\n"
                "  1. Connect Suunto Ambit watch to your computer using USB.\n"
                "  2. Run the program %s without parameters.\n"
                "     You will receive a list of Activities in the form: \"Activity type name\" UTC-Date-time\n"
                "  3. Run the program %s with UTC-Date-time of the Activity you need as parameters.\n"
                "     You will receive information about selected Activity in the format of the gpx.\n"
                "\n"
                ,
                argv[0],
                argv[0],
                argv[0]
            );
            return 0;
        }
        else 
        {
            time_stamp = argv[1];
        }
    }

    if ((ambit_object = libambit_detect()) != NULL) {
        libambit_device_info_get(ambit_object, &info);

        if (libambit_device_supported(ambit_object)) {
            fprintf(stderr, "Device: %s, serial: %s, FW version: %d.%d.%d\n", info.name, info.serial, info.fw_version[0], info.fw_version[1], info.fw_version[2] | (info.fw_version[3] << 8));

            if (libambit_device_status_get(ambit_object, &status) == 0) {
                fprintf(stderr, "Current charge: %d%%\n", status.charge);
            }
            else {
                fprintf(stderr, "Failed to read status\n");
            }

            if (libambit_personal_settings_get(ambit_object, &settings) == 0) {
            }
            else {
                fprintf(stderr, "Failed to read status\n");
            }

            libambit_log_read(ambit_object, log_skip_cb, log_data_cb, NULL, ambit_object);
        }
        else {
            fprintf(stderr, "Device: %s (fw_version: %d.%d.%d) is not supported yet!\n", info.name, info.fw_version[0], info.fw_version[1], info.fw_version[2] | (info.fw_version[3] << 8));
        }

        libambit_close(ambit_object);
    }
    else {
        fprintf(stderr, "No clock found, exiting\n");
    }

    return 0;
}

void print_header(ambit_log_header_t *log_header)
{
    printf("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\
<gpx creator=\"Ambit2GPX\" version=\"1.1\"\n\
    xmlns=\"http://www.topografix.com/GPX/1/1\"\n\
    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd http://www.garmin.com/xmlschemas/GpxExtensions/v3 http://www.garmin.com/xmlschemas/GpxExtensionsv3.xsd http://www.garmin.com/xmlschemas/TrackPointExtension/v1 http://www.garmin.com/xmlschemas/TrackPointExtensionv1.xsd\"\n\
    xmlns:gpxtpx=\"http://www.garmin.com/xmlschemas/TrackPointExtension/v1\"\n\
    xmlns:gpxx=\"http://www.garmin.com/xmlschemas/GpxExtensions/v3\">\n\
    <metadata>\n\
           <time>%s</time>\n\
    </metadata>\n\
    <trk>\n\
           <name>%s</name>\n\
           <trkseg>\n",
           time_string,
           activity_name
           );
}

void print_footer()
{
    printf("\
        </trkseg>\n\
    </trk>\n\
</gpx>\n"
    );
}

char* print_ambit_date_time_t_as_UTC_string(const ambit_date_time_t* dt, char* buf)
{
    if (buf == 0) buf = (char*)malloc(32);
    snprintf(
             buf,
             32,
             "%04u-%02u-%02uT%02u:%02u:%02uZ",
             dt->year,
             dt->month,
             dt->day,
             dt->hour,
             dt->minute,
             (unsigned)((1.0 * dt->msec) / 1000.0)
             );
    return buf;
}

int log_skip_cb(void *ambit_object, ambit_log_header_t *log_header)
{
    print_ambit_date_time_t_as_UTC_string(&log_header->date_time, time_string);
    
    float npp;
    sscanf(log_header->activity_name, "%f %s", &npp, activity_name);
    
    if (time_stamp == 0)
    {
        fprintf(stderr, "\"%s\" %s\n", activity_name, time_string);
        return 0;
    }

    return strcmp(time_stamp, time_string) == 0;
}

void process_ambit_log_sample_type_periodic(ambit_log_sample_t* sample)
{
    ambit_log_sample_periodic_value_t *value;
    int i;
    for (i=0; i<sample->u.periodic.value_count; i++) 
    {
        value = &sample->u.periodic.values[i];

        switch(value->type) 
        {
        case ambit_log_sample_periodic_type_latitude: 
            latitude = (double)value->u.latitude/10000000.0;
            break;
        case ambit_log_sample_periodic_type_longitude: 
            longitude = (double)value->u.longitude/10000000.0;
            break;
        case ambit_log_sample_periodic_type_distance: 
            if (value->u.distance != 0xffffffff) 
            {
                distance = value->u.distance;
            }
            break;
        case ambit_log_sample_periodic_type_speed: 
            if (value->u.speed != 0xffff) 
            {
                speed = (double)value->u.speed/100.0;
            }
            break;
        case ambit_log_sample_periodic_type_hr:
            if (value->u.hr != 0xff)
            {
                hr = value->u.hr;
            }
            break;
        case ambit_log_sample_periodic_type_time : break;
        case ambit_log_sample_periodic_type_gpsspeed : break;
        case ambit_log_sample_periodic_type_wristaccspeed : break;
        case ambit_log_sample_periodic_type_bikepodspeed : break;
        case ambit_log_sample_periodic_type_ehpe : break;
        case ambit_log_sample_periodic_type_evpe : break;
        case ambit_log_sample_periodic_type_altitude: 
            elevation = (double)value->u.altitude;
            break;
        case ambit_log_sample_periodic_type_abspressure : break;
        case ambit_log_sample_periodic_type_energy:
            if (value->u.energy)
            {
                energy = (double)value->u.energy/10.0;
            }
            break;
        case ambit_log_sample_periodic_type_temperature:
            if (value->u.temperature >= -1000 && value->u.temperature <= 1000) 
            {
                temperature = (double)value->u.temperature/10.0;
            }
            break;
        case ambit_log_sample_periodic_type_charge : break;
        case ambit_log_sample_periodic_type_gpsaltitude : break;
        case ambit_log_sample_periodic_type_gpsheading : break;
        case ambit_log_sample_periodic_type_gpshdop : break;
        case ambit_log_sample_periodic_type_gpsvdop : break;
        case ambit_log_sample_periodic_type_wristcadence : break;
        case ambit_log_sample_periodic_type_snr : break;
        case ambit_log_sample_periodic_type_noofsatellites : break;
        case ambit_log_sample_periodic_type_sealevelpressure: 
            sea_level_pressure = (int)round((double)value->u.sealevelpressure/10.0);
            break;
        case ambit_log_sample_periodic_type_verticalspeed:
            vertical_speed = (double)value->u.verticalspeed/100.0;
            break;
        case ambit_log_sample_periodic_type_cadence : break;
        case ambit_log_sample_periodic_type_bikepower : break;
        case ambit_log_sample_periodic_type_swimingstrokecnt : break;
        case ambit_log_sample_periodic_type_ruleoutput1 : break;
        case ambit_log_sample_periodic_type_ruleoutput2 : break;
        case ambit_log_sample_periodic_type_ruleoutput3 : break;
        case ambit_log_sample_periodic_type_ruleoutput4 : break;
        case ambit_log_sample_periodic_type_ruleoutput5 : break;
        }        
    }
}

void process_ambit_log_sample_type_gps_base(ambit_log_sample_t* sample)
{
    latitude = (double)sample->u.gps_base.latitude/10000000.0;
    longitude = (double)sample->u.gps_base.longitude/10000000.0;
    elevation = (double)sample->u.gps_base.altitude/100;
}

void process_ambit_log_sample_type_gps_small(ambit_log_sample_t* sample)
{
    latitude = (double)sample->u.gps_small.latitude/10000000.0;
    longitude = (double)sample->u.gps_small.longitude/10000000.0;
}

void process_ambit_log_sample_type_gps_tiny(ambit_log_sample_t* sample)
{
    latitude = (double)sample->u.gps_tiny.latitude/10000000.0;
    longitude = (double)sample->u.gps_tiny.longitude/10000000.0;
}

void process_ambit_log_sample_type_position(ambit_log_sample_t* sample)
{
    latitude = (double)sample->u.position.latitude/10000000.0;
    longitude = (double)sample->u.position.longitude/10000000.0;    
}

void write_track_point()
{
    if (latitude == 0.0 && longitude == 0.0) return;

    printf("\
            <trkpt lat=\"%f\" lon=\"%f\">\n\
                <ele>%.1f</ele>\n\
                <time>%s</time>\n\
                <extensions>\n\
                    <gpxtpx:TrackPointExtension>\n\
                        <gpxtpx:atemp>%d</gpxtpx:atemp>\n\
                        <gpxtpx:hr>%d</gpxtpx:hr>\n\
                    </gpxtpx:TrackPointExtension>\n\
                </extensions>\n\
            </trkpt>\n",
            latitude,
            longitude,
            elevation,
            time_string
            ///////////////////
            /// extensions
            , (int)temperature
            , hr
            );
}

void process_sample(ambit_log_sample_t* sample)
{
    switch(sample->type)
    {
    case ambit_log_sample_type_periodic:
        process_ambit_log_sample_type_periodic(sample);
        write_track_point();
        break;

    case ambit_log_sample_type_logpause : break;
    case ambit_log_sample_type_logrestart : break;
    case ambit_log_sample_type_ibi : break;
    case ambit_log_sample_type_ttff : break;
    case ambit_log_sample_type_distance_source : break;
    case ambit_log_sample_type_lapinfo : break;
    case ambit_log_sample_type_altitude_source : break;
    case ambit_log_sample_type_gps_base:
        process_ambit_log_sample_type_gps_base(sample);
        break;
    case ambit_log_sample_type_gps_small:
        process_ambit_log_sample_type_gps_small(sample);
        break;
    case ambit_log_sample_type_gps_tiny:
        process_ambit_log_sample_type_gps_tiny(sample);
        break;
    case ambit_log_sample_type_time : break;
    case ambit_log_sample_type_activity : break;
    case ambit_log_sample_type_position:
        process_ambit_log_sample_type_position(sample);
        break;
    case ambit_log_sample_type_unknown : break;
    }
}

void log_data_cb(void *object, ambit_log_entry_t *log_entry)
{
    print_header(&log_entry->header);
    
    int i;
    for (i=0; i<log_entry->header.samples_count; i++)
    {
        print_ambit_date_time_t_as_UTC_string(&log_entry->samples[i].utc_time, time_string);
        
        ambit_log_sample_t* sample = &log_entry->samples[i];
        process_sample(sample);
    }
    
    print_footer();
}
