/* -------------------------------------------------------------------------------

   Copyright (C) 1999-2007 id Software, Inc. and contributors.
   For a list of contributors, see the accompanying CONTRIBUTORS file.

   This file is part of GtkRadiant.

   GtkRadiant is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GtkRadiant is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GtkRadiant; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

   ----------------------------------------------------------------------------------

   This code has been altered significantly from its original form, to support
   several games based on the Quake III Arena engine, in the form of "Q3Map2."

   ------------------------------------------------------------------------------- */



/* dependencies */
#include "remap.h"



/*
   CreateSunLight() - ydnar
   this creates a sun light
 */

static void CreateSunLight( sun_t& sun ){
	/* fixup */
	value_maximize( sun.numSamples, 1 );

	/* set photons */
	const float photons = sun.photons / sun.numSamples;

	/* create the right number of suns */
	for ( int i = 0; i < sun.numSamples; ++i )
	{
		/* calculate sun direction */
		Vector3 direction;
		if ( i == 0 ) {
			direction = sun.direction;
		}
		else
		{
			/*
			    sun.direction[ 0 ] = cos( angle ) * cos( elevation );
			    sun.direction[ 1 ] = sin( angle ) * cos( elevation );
			    sun.direction[ 2 ] = sin( elevation );

			    xz_dist   = sqrt( x*x + z*z )
			    latitude  = atan2( xz_dist, y ) * RADIANS
			    longitude = atan2( x,       z ) * RADIANS
			 */

			const double d = sqrt( sun.direction[ 0 ] * sun.direction[ 0 ] + sun.direction[ 1 ] * sun.direction[ 1 ] );
			double angle = atan2( sun.direction[ 1 ], sun.direction[ 0 ] );
			double elevation = atan2( sun.direction[ 2 ], d );

			/* jitter the angles (loop to keep random sample within sun.deviance steridians) */
			float da, de;
			do
			{
				da = ( Random() * 2.0f - 1.0f ) * sun.deviance;
				de = ( Random() * 2.0f - 1.0f ) * sun.deviance;
			}
			while ( ( da * da + de * de ) > ( sun.deviance * sun.deviance ) );
			angle += da;
			elevation += de;

			/* debug code */
			//%	Sys_Printf( "%d: Angle: %3.4lf Elevation: %3.3lf\n", sun.numSamples, radians_to_degrees( angle ), radians_to_degrees( elevation ) );

			/* create new vector */
			direction = vector3_for_spherical( angle, elevation );
		}

		/* create a light */
		numSunLights++;
		light_t& light = lights.emplace_front();

		/* initialize the light */
		light.flags = LightFlags::DefaultSun;
		light.type = ELightType::Sun;
		light.fade = 1.0f;
		light.falloffTolerance = falloffTolerance;
		light.filterRadius = sun.filterRadius / sun.numSamples;
		light.style = noStyles ? LS_NORMAL : sun.style;

		/* set the light's position out to infinity */
		light.origin = direction * ( MAX_WORLD_COORD * 8.0f );    /* MAX_WORLD_COORD * 2.0f */

		/* set the facing to be the inverse of the sun direction */
		light.normal = -direction;
		light.dist = vector3_dot( light.origin, light.normal );

		/* set color and photons */
		light.color = sun.color;
		light.photons = photons * skyScale;
	}
}



class SkyProbes
{
	struct SkyProbe
	{
		Vector3 color;
		Vector3 normal;
	};
	std::vector<SkyProbe> m_probes;
public:
	SkyProbes() = default;
	SkyProbes( const String512& skyParmsImageBase ){
		if( !skyParmsImageBase.empty() ){
			std::vector<const image_t*> images;
			for( const auto suffix : { "_lf", "_rt", "_ft", "_bk", "_up", "_dn" } )
			{
				if( nullptr == images.emplace_back( ImageLoad( StringStream<64>( skyParmsImageBase, suffix ) ) ) ){
					Sys_Warning( "Couldn't find image %s\n", StringStream<64>( skyParmsImageBase, suffix ).c_str() );
					return;
				}
			}

			const size_t res = 64;
			m_probes.reserve( res * res * 6 );

	/* Q3 skybox (top view)
		   ^Y
		   |
		   bk
		lf    rt ->X
		   ft
	*/
			// postrotate everything from _rt (+x)
			const std::array<Matrix4, 6> transforms{ matrix4_scale_for_vec3( Vector3( -1, -1, 1 ) ),
													g_matrix4_identity,
													matrix4_rotation_for_sincos_z( -1, 0 ),
													matrix4_rotation_for_sincos_z( 1, 0 ),
													matrix4_rotation_for_sincos_y( -1, 0 ),
													matrix4_rotation_for_sincos_y( 1, 0 ) };

	/* img
			0-----> width / U
			|he
			|ig
			|ht
			V
	*/
			for( size_t i = 0; i < 6; ++i )
			{
				for( size_t u = 0; u < res; ++u )
				{
					for( size_t v = 0; v < res; ++v )
					{
						const Vector3 normal = vector3_normalised( Vector3( 1, 1 - u * 2.f / res, 1 - v * 2.f / res ) );
						Vector3 color( 0 );
						{
							const image_t& img = *images[i];
							const size_t w = img.width * u / res;
							const size_t w2 = std::max( w + 1, img.width * ( u + 1 ) / res );
							const size_t h = img.height * v / res;
							const size_t h2 = std::max( h + 1, img.height * ( v + 1 ) / res );
							for( size_t iw = w; iw < w2; ++iw )
							{
								for( size_t ih = h; ih < h2; ++ih )
								{
									color += vector3_from_array( img.at( iw, ih ) );
								}
							}
							color /= ( ( w2 - w ) * ( h2 - h ) );
						}
						color *= vector3_dot( normal, g_vector3_axis_x );
						m_probes.push_back( SkyProbe{ color / 255, matrix4_transformed_direction( transforms[i], normal ) } );
					}
				}
			}
		}
	}
	Vector3 sampleColour( const Vector3& direction, const Vector3& limitDirection ) const {
		Vector3 color( 0 );
		float weightSum = 0;
		for( const auto& probe : m_probes )
		{
			const double dot = vector3_dot( probe.normal, direction );
			if( dot > 0 && vector3_dot( probe.normal, limitDirection ) >= 0 ){
				color += probe.color * dot;
				weightSum += dot;
			}
		}
		return weightSum != 0? color / weightSum : color;
	}
	operator bool() const {
		return !m_probes.empty();
	}
};




/*
   CreateSkyLights() - ydnar
   simulates sky light with multiple suns
 */

static void CreateSkyLights( const skylight_t& skylight, const Vector3& color, float filterRadius, int style, const String512& skyParmsImageBase ){
	/* dummy check */
	if ( skylight.value <= 0.0f || skylight.iterations < 2 || skylight.horizon_min > skylight.horizon_max ) {
		return;
	}

	const SkyProbes probes = skylight.sample_color? SkyProbes( skyParmsImageBase ) : SkyProbes();
	const Vector3 probesLimitDirection = skylight.horizon_min >= 0? g_vector3_axis_z : skylight.horizon_max <= 0? -g_vector3_axis_z : Vector3( 0 );

	/* basic sun setup */
	sun_t sun;
	std::vector<sun_t> suns;
	sun.color = color;
	sun.deviance = 0.0f;
	sun.filterRadius = filterRadius;
	sun.numSamples = 1;
	sun.style = noStyles ? LS_NORMAL : style;

	/* setup */
	const int doBot = ( skylight.horizon_min == -90 );
	const int doTop = ( skylight.horizon_max == 90 );
	const int angleSteps = ( skylight.iterations - 1 ) * 4;
	const float eleStep = 90.0f / skylight.iterations;
	const float elevationStep = degrees_to_radians( eleStep );  /* skip elevation 0 */
	const float angleStep = degrees_to_radians( 360.0f / angleSteps );
	// const int elevationSteps = skylight.iterations - 1;
	const float eleMin = doBot? -90 + eleStep * 1.5 : skylight.horizon_min + eleStep * 0.5;
	const float eleMax = doTop? 90 - eleStep * 1.5 : skylight.horizon_max - eleStep * 0.5;
	const int elevationSteps = float_to_integer( 1 + std::max( 0.f, ( eleMax - eleMin ) / eleStep ) );

	/* calc individual sun brightness */
	const int numSuns = angleSteps * elevationSteps + doBot + doTop;
	sun.photons = skylight.value / numSuns * std::max( .25, ( skylight.horizon_max - skylight.horizon_min ) / 90.0 );
	suns.reserve( numSuns );

	/* iterate elevation */
	float elevation = degrees_to_radians( std::min( eleMin, float( skylight.horizon_max ) ) );
	float angle = 0.0f;
	for ( int i = 0; i < elevationSteps; ++i )
	{
		/* iterate angle */
		for ( int j = 0; j < angleSteps; ++j )
		{
			/* create sun */
			sun.direction = vector3_for_spherical( angle, elevation );
			if( probes )
				sun.color = probes.sampleColour( sun.direction, probesLimitDirection );
			suns.push_back( sun );

			/* move */
			angle += angleStep;
		}

		/* move */
		elevation += elevationStep;
		angle += angleStep / elevationSteps;
	}

	/* create vertical suns */
	if( doBot ){
		sun.direction = -g_vector3_axis_z;
		if( probes )
			sun.color = probes.sampleColour( sun.direction, probesLimitDirection );
		suns.push_back( sun );
	}
	if( doTop ){
		sun.direction = g_vector3_axis_z;
		if( probes )
			sun.color = probes.sampleColour( sun.direction, probesLimitDirection );
		suns.push_back( sun );
	}

	/* normalize colours */
	if( probes ){
		float max = 0;
		for( const auto& sun : suns )
		{
			value_maximize( max, sun.color[0] );
			value_maximize( max, sun.color[1] );
			value_maximize( max, sun.color[2] );
		}
		if( max != 0 )
			for( auto& sun : suns )
				sun.color /= max;
	}

	std::for_each( suns.begin(), suns.end(), CreateSunLight );
}



/*
   CreateEntityLights()
   creates lights from light entities
 */

static void CreateEntityLights(){
	/* go through entity list and find lights */
	for ( std::size_t i = 0; i < entities.size(); ++i )
	{
		/* get entity */
		const entity_t& e = entities[ i ];
		/* ydnar: check for lightJunior */
		bool junior;
		if ( e.classname_is( "lightJunior" ) ) {
			junior = true;
		}
		else if ( e.classname_prefixed( "light" ) ) {
			junior = false;
		}
		else{
			continue;
		}

		/* lights with target names (and therefore styles) are only parsed from BSP */
		if ( !strEmpty( e.valueForKey( "targetname" ) ) && i >= numBSPEntities ) {
			continue;
		}

		/* create a light */
		numPointLights++;
		light_t& light = lights.emplace_front();

		/* handle spawnflags */
		const int spawnflags = e.intForKey( "spawnflags" );

		LightFlags flags;
		/* ydnar: quake 3+ light behavior */
		if ( !wolfLight ) {
			/* set default flags */
			flags = LightFlags::DefaultQ3A;

			/* linear attenuation? */
			if ( spawnflags & 1 ) {
				flags |= LightFlags::AttenLinear;
				flags &= ~LightFlags::AttenAngle;
			}

			/* no angle attenuate? */
			if ( spawnflags & 2 ) {
				flags &= ~LightFlags::AttenAngle;
			}
		}

		/* ydnar: wolf light behavior */
		else
		{
			/* set default flags */
			flags = LightFlags::DefaultWolf;

			/* inverse distance squared attenuation? */
			if ( spawnflags & 1 ) {
				flags &= ~LightFlags::AttenLinear;
				flags |= LightFlags::AttenAngle;
			}

			/* angle attenuate? */
			if ( spawnflags & 2 ) {
				flags |= LightFlags::AttenAngle;
			}
		}

		/* other flags (borrowed from wolf) */

		/* wolf dark light? */
		if ( ( spawnflags & 4 ) || ( spawnflags & 8 ) ) {
			flags |= LightFlags::Dark;
		}

		/* nogrid? */
		if ( spawnflags & 16 ) {
			flags &= ~LightFlags::Grid;
		}

		/* junior? */
		if ( junior ) {
			flags |= LightFlags::Grid;
			flags &= ~LightFlags::Surfaces;
		}

		/* vortex: unnormalized? */
		if ( spawnflags & 32 ) {
			flags |= LightFlags::Unnormalized;
		}

		/* vortex: distance atten? */
		if ( spawnflags & 64 ) {
			flags |= LightFlags::AttenDistance;
		}

		/* store the flags */
		light.flags = flags;

		/* ydnar: set fade key (from wolf) */
		light.fade = 1.0f;
		if ( light.flags & LightFlags::AttenLinear ) {
			light.fade = e.floatForKey( "fade" );
			if ( light.fade == 0.0f ) {
				light.fade = 1.0f;
			}
		}

		/* ydnar: set angle scaling (from vlight) */
		light.angleScale = e.floatForKey( "_anglescale" );
		if ( light.angleScale != 0.0f ) {
			light.flags |= LightFlags::AttenAngle;
		}

		/* set origin */
		light.origin = e.vectorForKey( "origin" );
		e.read_keyvalue( light.style, "_style", "style" );
		if ( light.style < LS_NORMAL || light.style >= LS_NONE ) {
			Error( "Invalid lightstyle (%d) on entity %zu", light.style, i );
		}

		/* set light intensity */
		float intensity = 300.f;
		e.read_keyvalue( intensity, "_light", "light" );
		if ( intensity == 0.0f ) {
			intensity = 300.0f;
		}

		{	/* ydnar: set light scale (sof2) */
			float scale;
			if( e.read_keyvalue( scale, "scale" ) && scale != 0.f )
				intensity *= scale;
		}

		/* ydnar: get deviance and samples */
		const float deviance = std::max( 0.f, e.floatForKey( "_deviance", "_deviation", "_jitter" ) );
		const int numSamples = std::max( 1, e.intForKey( "_samples" ) );

		intensity /= numSamples;

		{	/* ydnar: get filter radius */
			light.filterRadius = std::max( 0.f, e.floatForKey( "_filterradius", "_filteradius", "_filter" ) );
		}

		/* set light color */
		if ( e.read_keyvalue( light.color, "_color" ) ) {
			if ( colorsRGB ) {
				light.color[0] = Image_LinearFloatFromsRGBFloat( light.color[0] );
				light.color[1] = Image_LinearFloatFromsRGBFloat( light.color[1] );
				light.color[2] = Image_LinearFloatFromsRGBFloat( light.color[2] );
			}
			if ( !( light.flags & LightFlags::Unnormalized ) ) {
				ColorNormalize( light.color );
			}
		}
		else{
			light.color.set( 1 );
		}


		if( !e.read_keyvalue( light.extraDist, "_extradist" ) )
			light.extraDist = extraDist;

		light.photons = intensity;

		light.type = ELightType::Point;

		/* set falloff threshold */
		light.falloffTolerance = falloffTolerance / numSamples;

		/* lights with a target will be spotlights */
		const char *target;
		if ( e.read_keyvalue( target, "target" ) ) {
			/* get target */
			const entity_t *e2 = FindTargetEntity( target );
			if ( e2 == NULL ) {
				Sys_Warning( "light at (%i %i %i) has missing target\n",
				             (int) light.origin[ 0 ], (int) light.origin[ 1 ], (int) light.origin[ 2 ] );
				light.photons *= pointScale;
			}
			else
			{
				/* not a point light */
				numPointLights--;
				numSpotLights++;

				/* make a spotlight */
				light.normal = e2->vectorForKey( "origin" ) - light.origin;
				float dist = VectorNormalize( light.normal );
				float radius = e.floatForKey( "radius" );
				if ( !radius ) {
					radius = 64;
				}
				if ( !dist ) {
					dist = 64;
				}
				light.radiusByDist = ( radius + 16 ) / dist;
				light.type = ELightType::Spot;

				/* ydnar: wolf mods: spotlights always use nonlinear + angle attenuation */
				light.flags &= ~LightFlags::AttenLinear;
				light.flags |= LightFlags::AttenAngle;
				light.fade = 1.0f;

				/* ydnar: is this a sun? */
				if ( e.boolForKey( "_sun" ) ) {
					/* not a spot light */
					numSpotLights--;

					/* make a sun */
					sun_t sun{};
					sun.direction = -light.normal;
					sun.color = light.color;
					sun.photons = intensity;
					sun.deviance = degrees_to_radians( deviance );
					sun.numSamples = numSamples;
					sun.style = noStyles ? LS_NORMAL : light.style;

					/* free original light before sun insertion */
					lights.pop_front();

					/* make a sun light */
					CreateSunLight( sun );

					/* skip the rest of this love story */
					continue;
				}
				else
				{
					light.photons *= spotScale;
				}
			}
		}
		else{
			light.photons *= pointScale;
		}

		/* jitter the light */
		for ( int j = 1; j < numSamples; j++ )
		{
			/* create a light */
			light_t& light2 = lights.emplace_front( light );

			/* add to counts */
			if ( light.type == ELightType::Spot ) {
				numSpotLights++;
			}
			else{
				numPointLights++;
			}

			/* jitter it */
			light2.origin[ 0 ] = light.origin[ 0 ] + ( Random() * 2.0f - 1.0f ) * deviance;
			light2.origin[ 1 ] = light.origin[ 1 ] + ( Random() * 2.0f - 1.0f ) * deviance;
			light2.origin[ 2 ] = light.origin[ 2 ] + ( Random() * 2.0f - 1.0f ) * deviance;
		}
	}
}



/*
   CreateSurfaceLights() - ydnar
   this hijacks the radiosity code to generate surface lights for first pass
 */

#define APPROX_BOUNCE   1.0f

static void CreateSurfaceLights(){
	
}



/*
   SetEntityOrigins()
   find the offset values for inline models
 */

static void SetEntityOrigins(){

}



/*
   PointToPolygonFormFactor()
   calculates the area over a point/normal hemisphere a winding covers
   ydnar: fixme: there has to be a faster way to calculate this
   without the expensive per-vert sqrts and transcendental functions
   ydnar 2002-09-30: added -faster switch because only 19% deviance > 10%
   between this and the approximation
 */

float PointToPolygonFormFactor( const Vector3& point, const Vector3& normal, const winding_t& w ){
	Vector3 dirs[ MAX_POINTS_ON_WINDING ];
	double total = 0;


	/* this is expensive */
	for ( size_t i = 0; i < w.size(); ++i )
	{
		dirs[ i ] = w[ i ] - point;
		VectorFastNormalize( dirs[ i ] );
	}

	/* duplicate first vertex to avoid mod operation */
	dirs[ w.size() ] = dirs[ 0 ];

	/* calculcate relative area */
	for ( size_t i = 0; i < w.size(); ++i )
	{
		/* get a triangle */
		Vector3 triNormal = vector3_cross( dirs[ i ], dirs[ i + 1 ] );
		if ( VectorFastNormalize( triNormal ) < 0.0001f ) {
			continue;
		}

		/* get the angle */
		/* roundoff can cause slight creep, which gives an IND from acos, thus clamp */
		const double angle = acos( std::clamp( vector3_dot( dirs[ i ], dirs[ i + 1 ] ), -1.0, 1.0 ) );

		const double facing = vector3_dot( normal, triNormal );
		total += facing * angle;

		/* ydnar: this was throwing too many errors with radiosity + crappy maps. ignoring it. */
		if ( total > 6.3 || total < -6.3 ) {
			return 0.0f;
		}
	}

	/* now in the range of 0 to 1 over the entire incoming hemisphere */
	//%	total /= (2.0f * 3.141592657f);
	return total * c_inv_2pi;
}



/*
   LightContributionTosample()
   determines the amount of light reaching a sample (luxel or vertex) from a given light
 */

#include <algorithm>

int LightContributionToSample( trace_t *trace ){
	float angle;
	float add;
	float dist;
	float addDeluxe = 0.0f, addDeluxeBounceScale = 0.25f;
	bool angledDeluxe = true;
	float colorBrightness;
	bool doAddDeluxe = true;

	/* get light */
	const light_t *light = trace->light;

	/* clear color */
	trace->forceSubsampling = 0.0f; /* to make sure */
	trace->color.set( 0 );
	trace->directionContribution.set( 0 );

	colorBrightness = RGBTOGRAY( light->color ) * ( 1.0f / 255.0f );

	/* ydnar: early out */
	if ( !( light->flags & LightFlags::Surfaces ) || light->envelope <= 0.0f ) {
		return 0;
	}

	/* do some culling checks */
	if ( light->type != ELightType::Sun ) {
		/* MrE: if the light is behind the surface */
		if ( !trace->twoSided ) {
			if ( vector3_dot( light->origin, trace->normal ) - vector3_dot( trace->origin, trace->normal ) < 0.0f ) {
				return 0;
			}
		}

		/* ydnar: test pvs */
		if ( !ClusterVisible( trace->cluster, light->cluster ) ) {
			return 0;
		}
	}

	/* exact point to polygon form factor */
	if ( light->type == ELightType::Area ) {
		float factor;
		float d;
		Vector3 pushedOrigin;

		/* project sample point into light plane */
		d = vector3_dot( trace->origin, light->normal ) - light->dist;
		if ( d < 3.0f ) {
			/* sample point behind plane? */
			if ( !( light->flags & LightFlags::Twosided ) && d < -1.0f ) {
				return 0;
			}

			/* sample plane coincident? */
			if ( d > -3.0f && vector3_dot( trace->normal, light->normal ) > 0.9f ) {
				return 0;
			}
		}

		/* nudge the point so that it is clearly forward of the light */
		/* so that surfaces meeting a light emitter don't get black edges */
		if ( d > -8.0f && d < 8.0f ) {
			pushedOrigin = trace->origin + light->normal * ( 8.0f - d );
		}
		else{
			pushedOrigin = trace->origin;
		}

		/* get direction and distance */
		trace->end = light->origin;
		dist = SetupTrace( trace );
		if ( dist >= light->envelope ) {
			return 0;
		}

		/* ptpff approximation */
		if ( faster ) {
			/* angle attenuation */
			angle = vector3_dot( trace->normal, trace->direction );

			/* twosided lighting */
			if ( trace->twoSided && angle < 0 ) {
				angle = -angle;

				/* no deluxemap contribution from "other side" light */
				doAddDeluxe = false;
			}

			/* attenuate */
			angle *= -vector3_dot( light->normal, trace->direction );
			if ( angle == 0.0f ) {
				return 0;
			}
			else if ( angle < 0.0f &&
			          ( trace->twoSided || ( light->flags & LightFlags::Twosided ) ) ) {
				angle = -angle;

				/* no deluxemap contribution from "other side" light */
				doAddDeluxe = false;
			}

			/* clamp the distance to prevent super hot spots */
			dist = std::max( 16.0f, sqrtf( dist * dist + light->extraDist * light->extraDist ) );

			add = light->photons / ( dist * dist ) * angle;

			if ( deluxemap ) {
				if ( angledDeluxe ) {
					addDeluxe = light->photons / ( dist * dist ) * angle;
				}
				else{
					addDeluxe = light->photons / ( dist * dist );
				}
			}
		}
		else
		{
			/* calculate the contribution */
			factor = PointToPolygonFormFactor( pushedOrigin, trace->normal, light->w );
			if ( factor == 0.0f ) {
				return 0;
			}
			else if ( factor < 0.0f ) {
				/* twosided lighting */
				if ( trace->twoSided || ( light->flags & LightFlags::Twosided ) ) {
					factor = -factor;

					/* push light origin to other side of the plane */
					trace->end = light->origin - light->normal * 2.f;
					dist = SetupTrace( trace );
					if ( dist >= light->envelope ) {
						return 0;
					}

					/* no deluxemap contribution from "other side" light */
					doAddDeluxe = false;
				}
				else{
					return 0;
				}
			}

			/* also don't deluxe if the direction is on the wrong side */
			if ( vector3_dot( trace->normal, trace->direction ) < 0 ) {
				/* no deluxemap contribution from "other side" light */
				doAddDeluxe = false;
			}

			/* ydnar: moved to here */
			add = factor * light->add;

			if ( deluxemap ) {
				addDeluxe = add;
			}
		}
	}

	/* point/spot lights */
	else if ( light->type == ELightType::Point || light->type == ELightType::Spot ) {
		/* get direction and distance */
		trace->end = light->origin;
		dist = SetupTrace( trace );
		if ( dist >= light->envelope ) {
			return 0;
		}

		/* clamp the distance to prevent super hot spots */
		dist = std::max( 16.0f, sqrtf( dist * dist + light->extraDist * light->extraDist ) );

		/* angle attenuation */
		if ( light->flags & LightFlags::AttenAngle ) {
			/* standard Lambert attenuation */
			float dot = vector3_dot( trace->normal, trace->direction );

			/* twosided lighting */
			if ( trace->twoSided && dot < 0 ) {
				dot = -dot;

				/* no deluxemap contribution from "other side" light */
				doAddDeluxe = false;
			}

			/* jal: optional half Lambert attenuation (http://developer.valvesoftware.com/wiki/Half_Lambert) */
			if ( lightAngleHL ) {
				if ( dot > 0.001f ) { // skip coplanar
					value_minimize( dot, 1.0f );
					dot = ( dot * 0.5f ) + 0.5f;
					dot *= dot;
				}
				else{
					dot = 0;
				}
			}

			angle = dot;
		}
		else{
			angle = 1.0f;
		}

		if ( light->angleScale != 0.0f ) {
			angle /= light->angleScale;
			value_minimize( angle, 1.0f );
		}

		/* attenuate */
		if ( light->flags & LightFlags::AttenLinear ) {
			add = std::max( 0.f, angle * light->photons * linearScale - ( dist * light->fade ) );

			if ( deluxemap ) {
				if ( angledDeluxe ) {
					addDeluxe = angle * light->photons * linearScale - ( dist * light->fade );
				}
				else{
					addDeluxe = light->photons * linearScale - ( dist * light->fade );
				}

				value_maximize( addDeluxe, 0.0f );
			}
		}
		else
		{
			add = std::max( 0.f, ( light->photons / ( dist * dist ) ) * angle );

			if ( deluxemap ) {
				if ( angledDeluxe ) {
					addDeluxe = ( light->photons / ( dist * dist ) ) * angle;
				}
				else{
					addDeluxe = ( light->photons / ( dist * dist ) );
				}
			}

			value_maximize( addDeluxe, 0.0f );
		}

		/* handle spotlights */
		if ( light->type == ELightType::Spot ) {
			/* do cone calculation */
			const float distByNormal = -vector3_dot( trace->displacement, light->normal );
			if ( distByNormal < 0.0f ) {
				return 0;
			}
			const Vector3 pointAtDist = light->origin + light->normal * distByNormal;
			const float radiusAtDist = light->radiusByDist * distByNormal;
			const Vector3 distToSample = trace->origin - pointAtDist;
			const float sampleRadius = vector3_length( distToSample );

			/* outside the cone */
			if ( sampleRadius >= radiusAtDist ) {
				return 0;
			}

			/* attenuate */
			if ( sampleRadius > ( radiusAtDist - 32.0f ) ) {
				add *= ( ( radiusAtDist - sampleRadius ) / 32.0f );
				value_maximize( add, 0.0f );

				addDeluxe *= ( ( radiusAtDist - sampleRadius ) / 32.0f );
				value_maximize( addDeluxe, 0.0f );
			}
		}
	}

	/* ydnar: sunlight */
	else if ( light->type == ELightType::Sun ) {
		/* get origin and direction */
		trace->end = trace->origin + light->origin;
		dist = SetupTrace( trace );

		/* angle attenuation */
		if ( light->flags & LightFlags::AttenAngle ) {
			/* standard Lambert attenuation */
			float dot = vector3_dot( trace->normal, trace->direction );

			/* twosided lighting */
			if ( trace->twoSided && dot < 0 ) {
				dot = -dot;

				/* no deluxemap contribution from "other side" light */
				doAddDeluxe = false;
			}

			/* jal: optional half Lambert attenuation (http://developer.valvesoftware.com/wiki/Half_Lambert) */
			if ( lightAngleHL ) {
				if ( dot > 0.001f ) { // skip coplanar
					value_minimize( dot, 1.0f );
					dot = ( dot * 0.5f ) + 0.5f;
					dot *= dot;
				}
				else{
					dot = 0;
				}
			}

			angle = dot;
		}
		else{
			angle = 1.0f;
		}

		/* attenuate */
		add = light->photons * angle;

		if ( deluxemap ) {
			if ( angledDeluxe ) {
				addDeluxe = light->photons * angle;
			}
			else{
				addDeluxe = light->photons;
			}

			value_maximize( addDeluxe, 0.0f );
		}

		if ( add <= 0.0f ) {
			return 0;
		}

		addDeluxe *= colorBrightness;

		if ( bouncing ) {
			addDeluxe *= addDeluxeBounceScale;
			value_maximize( addDeluxe, 0.00390625f );
		}

		trace->directionContribution = trace->direction * addDeluxe;

		/* setup trace */
		trace->testAll = true;
		trace->color = light->color * add;

		/* trace to point */
		if ( trace->testOcclusion && !trace->forceSunlight ) {
			/* trace */
			TraceLine( trace );
			trace->forceSubsampling *= add;
			if ( !( trace->compileFlags & C_SKY ) || trace->opaque ) {
				trace->color.set( 0 );
				trace->directionContribution.set( 0 );

				return -1;
			}
		}

		/* return to sender */
		return 1;
	}
	else{
		Error( "Light of undefined type!" );
	}

	/* ydnar: changed to a variable number */
	if ( add <= 0.0f || ( add <= light->falloffTolerance && ( light->flags & LightFlags::FastActual ) ) ) {
		return 0;
	}

	addDeluxe *= colorBrightness;

	/* hack land: scale down the radiosity contribution to light directionality.
	   Deluxemaps fusion many light directions into one. In a rtl process all lights
	   would contribute individually to the bump map, so several light sources together
	   would make it more directional (example: a yellow and red lights received from
	   opposing sides would light one side in red and the other in blue, adding
	   the effect of 2 directions applied. In the deluxemapping case, this 2 lights would
	   neutralize each other making it look like having no direction.
	   Same thing happens with radiosity. In deluxemapping case the radiosity contribution
	   is modifying the direction applied from directional lights, making it go closer and closer
	   to the surface normal the bigger is the amount of radiosity received.
	   So, for preserving the directional lights contributions, we scale down the radiosity
	   contribution. It's a hack, but there's a reason behind it */
	if ( bouncing ) {
		addDeluxe *= addDeluxeBounceScale;
		/* better NOT increase it beyond the original value
		   if( addDeluxe < 0.00390625f )
		    addDeluxe = 0.00390625f;
		 */
	}

	if ( doAddDeluxe ) {
		trace->directionContribution = trace->direction * addDeluxe;
	}

	/* setup trace */
	trace->testAll = false;
	trace->color = light->color * add;

	/* raytrace */
	TraceLine( trace );
	trace->forceSubsampling *= add;
	if ( trace->passSolid || trace->opaque ) {
		trace->color.set( 0 );
		trace->directionContribution.set( 0 );

		return -1;
	}

	/* return to sender */
	return 1;
}



/*
   LightingAtSample()
   determines the amount of light reaching a sample (luxel or vertex)
 */

void LightingAtSample( trace_t *trace, byte styles[ MAX_LIGHTMAPS ], Vector3 (&colors)[ MAX_LIGHTMAPS ] ){
	int i, lightmapNum;


	/* clear colors */
	for ( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; lightmapNum++ )
		colors[ lightmapNum ].set( 0 );

	/* ydnar: normalmap */
	if ( normalmap ) {
		colors[ 0 ] = ( trace->normal + Vector3( 1 ) ) * 127.5f;
		return;
	}

	/* ydnar: don't bounce ambient all the time */
	if ( !bouncing ) {
		colors[ 0 ] = ambientColor;
	}

	/* ydnar: trace to all the list of lights pre-stored in tw */
	for ( i = 0; i < trace->numLights && trace->lights[ i ] != NULL; i++ )
	{
		/* set light */
		trace->light = trace->lights[ i ];

		/* style check */
		for ( lightmapNum = 0; lightmapNum < MAX_LIGHTMAPS; lightmapNum++ )
		{
			if ( styles[ lightmapNum ] == trace->light->style ||
			     styles[ lightmapNum ] == LS_NONE ) {
				break;
			}
		}

		/* max of MAX_LIGHTMAPS (4) styles allowed to hit a sample */
		if ( lightmapNum >= MAX_LIGHTMAPS ) {
			continue;
		}

		/* sample light */
		LightContributionToSample( trace );
		if ( trace->color == g_vector3_identity ) {
			continue;
		}

		/* handle negative light */
		if ( trace->light->flags & LightFlags::Negative ) {
			vector3_negate( trace->color );
		}

		/* set style */
		styles[ lightmapNum ] = trace->light->style;

		/* add it */
		colors[ lightmapNum ] += trace->color;

		/* cheap mode */
		if ( cheap &&
		     colors[ 0 ][ 0 ] >= 255.0f &&
		     colors[ 0 ][ 1 ] >= 255.0f &&
		     colors[ 0 ][ 2 ] >= 255.0f ) {
			break;
		}
	}
}



/*
   LightContributionToPoint()
   for a given light, how much light/color reaches a given point in space (with no facing)
   note: this is similar to LightContributionToSample() but optimized for omnidirectional sampling
 */

static bool LightContributionToPoint( trace_t *trace ){
	float add, dist;

	/* get light */
	const light_t *light = trace->light;

	/* clear color */
	trace->color.set( 0 );

	/* ydnar: early out */
	if ( !( light->flags & LightFlags::Grid ) || light->envelope <= 0.0f ) {
		return false;
	}

	/* is this a sun? */
	if ( light->type != ELightType::Sun ) {
		/* sun only? */
		if ( sunOnly ) {
			return false;
		}

		/* test pvs */
		if ( !ClusterVisible( trace->cluster, light->cluster ) ) {
			return false;
		}
	}

	/* ydnar: check origin against light's pvs envelope */
	if ( !light->minmax.test( trace->origin ) ) {
		gridBoundsCulled++;
		return false;
	}

	/* set light origin */
	if ( light->type == ELightType::Sun ) {
		trace->end = trace->origin + light->origin;
	}
	else{
		trace->end = light->origin;
	}

	/* set direction */
	dist = SetupTrace( trace );

	/* test envelope */
	if ( dist > light->envelope ) {
		gridEnvelopeCulled++;
		return false;
	}

	/* ptpff approximation */
	if ( light->type == ELightType::Area && faster ) {
		/* clamp the distance to prevent super hot spots */
		dist = std::max( 16.0f, sqrtf( dist * dist + light->extraDist * light->extraDist ) );

		/* attenuate */
		add = light->photons / ( dist * dist );
	}

	/* exact point to polygon form factor */
	else if ( light->type == ELightType::Area ) {
		float factor, d;
		Vector3 pushedOrigin;


		/* see if the point is behind the light */
		d = vector3_dot( trace->origin, light->normal ) - light->dist;
		if ( !( light->flags & LightFlags::Twosided ) && d < -1.0f ) {
			return false;
		}

		/* nudge the point so that it is clearly forward of the light */
		/* so that surfaces meeting a light emiter don't get black edges */
		if ( d > -8.0f && d < 8.0f ) {
			pushedOrigin = trace->origin + light->normal * ( 8.0f - d );
		}
		else{
			pushedOrigin = trace->origin;
		}

		/* calculate the contribution (ydnar 2002-10-21: [bug 642] bad normal calc) */
		factor = PointToPolygonFormFactor( pushedOrigin, trace->direction, light->w );
		if ( factor == 0.0f ) {
			return false;
		}
		else if ( factor < 0.0f ) {
			if ( light->flags & LightFlags::Twosided ) {
				factor = -factor;
			}
			else{
				return false;
			}
		}

		/* ydnar: moved to here */
		add = factor * light->add;
	}

	/* point/spot lights */
	else if ( light->type == ELightType::Point || light->type == ELightType::Spot ) {
		/* clamp the distance to prevent super hot spots */
		dist = std::max( 16.0f, sqrtf( dist * dist + light->extraDist * light->extraDist ) );

		/* attenuate */
		if ( light->flags & LightFlags::AttenLinear ) {
			add = std::max( 0.f, light->photons * linearScale - ( dist * light->fade ) );
		}
		else{
			add = light->photons / ( dist * dist );
		}

		/* handle spotlights */
		if ( light->type == ELightType::Spot ) {
			/* do cone calculation */
			const float distByNormal = -vector3_dot( trace->displacement, light->normal );
			if ( distByNormal < 0.0f ) {
				return false;
			}
			const Vector3 pointAtDist = light->origin + light->normal * distByNormal;
			const float radiusAtDist = light->radiusByDist * distByNormal;
			const Vector3 distToSample = trace->origin - pointAtDist;
			const float sampleRadius = vector3_length( distToSample );

			/* outside the cone */
			if ( sampleRadius >= radiusAtDist ) {
				return false;
			}

			/* attenuate */
			if ( sampleRadius > ( radiusAtDist - 32.0f ) ) {
				add *= ( ( radiusAtDist - sampleRadius ) / 32.0f );
			}
		}
	}

	/* ydnar: sunlight */
	else if ( light->type == ELightType::Sun ) {
		/* attenuate */
		add = light->photons;
		if ( add <= 0.0f ) {
			return false;
		}

		/* setup trace */
		trace->testAll = true;
		trace->color = light->color * add;

		/* trace to point */
		if ( trace->testOcclusion && !trace->forceSunlight ) {
			/* trace */
			TraceLine( trace );
			if ( !( trace->compileFlags & C_SKY ) || trace->opaque ) {
				trace->color.set( 0 );
				return false;
			}
		}

		/* return to sender */
		return true;
	}

	/* unknown light type */
	else{
		return false;
	}

	/* ydnar: changed to a variable number */
	if ( add <= 0.0f || ( add <= light->falloffTolerance && ( light->flags & LightFlags::FastActual ) ) ) {
		return false;
	}

	/* setup trace */
	trace->testAll = false;
	trace->color = light->color * add;

	/* trace */
	TraceLine( trace );
	if ( trace->passSolid ) {
		trace->color.set( 0 );
		return false;
	}

	/* we have a valid sample */
	return true;
}



/*
   TraceGrid()
   grid samples are for quickly determining the lighting
   of dynamically placed entities in the world
 */

#define MAX_CONTRIBUTIONS   32768

struct contribution_t
{
	Vector3 dir;
	Vector3 color;
	Vector3 ambient;
	int style;
};

static void TraceGrid( int num ){
	int i, j, x, y, z, mod, numCon, numStyles;
	float d, step;
	Vector3 cheapColor, thisdir;
	rawGridPoint_t          *gp;
	bspGridPoint_t          *bgp;
	contribution_t contributions[ MAX_CONTRIBUTIONS ];
	trace_t trace;

	/* get grid points */
	gp = &rawGridPoints[ num ];
	bgp = &bspGridPoints[ num ];

	/* get grid origin */
	mod = num;
	z = mod / ( gridBounds[ 0 ] * gridBounds[ 1 ] );
	mod -= z * ( gridBounds[ 0 ] * gridBounds[ 1 ] );
	y = mod / gridBounds[ 0 ];
	mod -= y * gridBounds[ 0 ];
	x = mod;

	trace.origin = gridMins + Vector3( x, y, z ) * gridSize;

	/* set inhibit sphere */
	trace.inhibitRadius = gridSize[ vector3_max_abs_component_index( gridSize ) ] * 0.5f;

	/* find point cluster */
	trace.cluster = ClusterForPointExt( trace.origin, GRID_EPSILON );
	if ( trace.cluster < 0 ) {
		/* try to nudge the origin around to find a valid point */
		const Vector3 baseOrigin( trace.origin );
		for ( step = 0; ( step += 0.005 ) <= 1.0; )
		{
			trace.origin = baseOrigin;
			trace.origin[ 0 ] += step * ( Random() - 0.5 ) * gridSize[0];
			trace.origin[ 1 ] += step * ( Random() - 0.5 ) * gridSize[1];
			trace.origin[ 2 ] += step * ( Random() - 0.5 ) * gridSize[2];

			/* ydnar: changed to find cluster num */
			trace.cluster = ClusterForPointExt( trace.origin, VERTEX_EPSILON );
			if ( trace.cluster >= 0 ) {
				break;
			}
		}

		/* can't find a valid point at all */
		if ( step > 1.0 ) {
			return;
		}
	}

	/* setup trace */
	trace.testOcclusion = !noTrace;
	trace.forceSunlight = false;
	trace.recvShadows = WORLDSPAWN_RECV_SHADOWS;
	trace.numSurfaces = 0;
	trace.surfaces = NULL;
	trace.numLights = 0;
	trace.lights = NULL;

	/* clear */
	numCon = 0;
	cheapColor.set( 0 );

	/* trace to all the lights, find the major light direction, and divide the
	   total light between that along the direction and the remaining in the ambient */
	for ( const light_t& light : lights )
	{
		trace.light = &light;
		float addSize;


		/* sample light */
		if ( !LightContributionToPoint( &trace ) ) {
			continue;
		}

		/* handle negative light */
		if ( trace.light->flags & LightFlags::Negative ) {
			vector3_negate( trace.color );
		}

		/* add a contribution */
		contributions[ numCon ].color = trace.color;
		contributions[ numCon ].dir = trace.direction;
		contributions[ numCon ].ambient.set( 0 );
		contributions[ numCon ].style = trace.light->style;
		numCon++;

		/* push average direction around */
		addSize = vector3_length( trace.color );
		gp->dir += trace.direction * addSize;

		/* stop after a while */
		if ( numCon >= ( MAX_CONTRIBUTIONS - 1 ) ) {
			break;
		}

		/* ydnar: cheap mode */
		cheapColor += trace.color;
		if ( cheapgrid && cheapColor[ 0 ] >= 255.0f && cheapColor[ 1 ] >= 255.0f && cheapColor[ 2 ] >= 255.0f ) {
			break;
		}
	}

	/////// Floodlighting for point //////////////////
	//do our floodlight ambient occlusion loop, and add a single contribution based on the brightest dir
	if ( floodlighty ) {
		int k;
		float addSize, f;
		Vector3 dir = g_vector3_axis_z;
		float ambientFrac = 0.25f;

		trace.testOcclusion = true;
		trace.forceSunlight = false;
		trace.inhibitRadius = DEFAULT_INHIBIT_RADIUS;
		trace.testAll = true;

		for ( k = 0; k < 2; k++ )
		{
			if ( k == 0 ) { // upper hemisphere
				trace.normal = g_vector3_axis_z;
			}
			else //lower hemisphere
			{
				trace.normal = -g_vector3_axis_z;
			}

			f = FloodLightForSample( &trace, floodlightDistance, floodlight_lowquality );

			/* add a fraction as pure ambient, half as top-down direction */
			contributions[ numCon ].color = floodlightRGB * ( floodlightIntensity * f * ( 1.0f - ambientFrac ) );

			contributions[ numCon ].ambient = floodlightRGB * ( floodlightIntensity * f * ambientFrac );

			contributions[ numCon ].dir = dir;

			contributions[ numCon ].style = 0;

			/* push average direction around */
			addSize = vector3_length( contributions[ numCon ].color );
			gp->dir += dir * addSize;

			numCon++;
		}
	}
	/////////////////////

	/* normalize to get primary light direction */
	thisdir = VectorNormalized( gp->dir );

	/* now that we have identified the primary light direction,
	   go back and separate all the light into directed and ambient */

	numStyles = 1;
	for ( i = 0; i < numCon; i++ )
	{
		/* get relative directed strength */
		d = vector3_dot( contributions[ i ].dir, thisdir );
		/* we map 1 to gridDirectionality, and 0 to gridAmbientDirectionality */
		d = std::max( 0.f, gridAmbientDirectionality + d * ( gridDirectionality - gridAmbientDirectionality ) );

		/* find appropriate style */
		for ( j = 0; j < numStyles; j++ )
		{
			if ( gp->styles[ j ] == contributions[ i ].style ) {
				break;
			}
		}

		/* style not found? */
		if ( j >= numStyles ) {
			/* add a new style */
			if ( numStyles < MAX_LIGHTMAPS ) {
				gp->styles[ numStyles ] = contributions[ i ].style;
				bgp->styles[ numStyles ] = contributions[ i ].style;
				numStyles++;
				//%	Sys_Printf( "(%d, %d) ", num, contributions[ i ].style );
			}

			/* fallback */
			else{
				j = 0;
			}
		}

		/* add the directed color */
		gp->directed[ j ] += contributions[ i ].color * d;

		/* ambient light will be at 1/4 the value of directed light */
		/* (ydnar: nuke this in favor of more dramatic lighting?) */
		/* (PM: how about actually making it work? d=1 when it got here for single lights/sun :P */
//		d = 0.25f;
		/* (Hobbes: always setting it to .25 is hardly any better) */
		d = 0.25f * ( 1.0f - d );
		gp->ambient[ j ] += contributions[ i ].color * d;

		gp->ambient[ j ] += contributions[ i ].ambient;

/*
 * div0:
 * the total light average = ambient value + 0.25 * sum of all directional values
 * we can also get the total light average as 0.25 * the sum of all contributions
 *
 * 0.25 * sum(contribution_i) == ambient + 0.25 * sum(d_i contribution_i)
 *
 * THIS YIELDS:
 * ambient == 0.25 * sum((1 - d_i) contribution_i)
 *
 * So, 0.25f * (1.0f - d) IS RIGHT. If you want to tune it, tune d BEFORE.
 */
	}


	/* store off sample */
	for ( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
#if 0
		/* do some fudging to keep the ambient from being too low (2003-07-05: 0.25 -> 0.125) */
		if ( !bouncing ) {
			VectorMA( gp->ambient[ i ], 0.125f, gp->directed[ i ], gp->ambient[ i ] );
		}
#endif

		/* set minimum light and copy off to bytes */
		Vector3 color = gp->ambient[ i ];
		for ( j = 0; j < 3; j++ )
			value_maximize( color[ j ], minGridLight[ j ] );

		/* vortex: apply gridscale and gridambientscale here */
		bgp->ambient[ i ] = ColorToBytes( color, gridScale * gridAmbientScale );
		bgp->directed[ i ] = ColorToBytes( gp->directed[ i ], gridScale );
		/*
		 * HACK: if there's a non-zero directed component, this
		 * lightgrid cell is useful. However, q3 skips grid
		 * cells with zero ambient. So let's force ambient to be
		 * nonzero unless directed is zero too.
		 */
		 if( bgp->ambient[i][0] + bgp->ambient[i][1] + bgp->ambient[i][2] == 0
		&& bgp->directed[i][0] + bgp->directed[i][1] + bgp->directed[i][2] != 0 )
			bgp->ambient[i].set( 1 );
	}

	/* debug code */
	#if 0
	//%	Sys_FPrintf( SYS_VRB, "%10d %10d %10d ", &gp->ambient[ 0 ][ 0 ], &gp->ambient[ 0 ][ 1 ], &gp->ambient[ 0 ][ 2 ] );
	Sys_FPrintf( SYS_VRB, "%9d Amb: (%03.1f %03.1f %03.1f) Dir: (%03.1f %03.1f %03.1f)\n",
	             num,
	             gp->ambient[ 0 ][ 0 ], gp->ambient[ 0 ][ 1 ], gp->ambient[ 0 ][ 2 ],
	             gp->directed[ 0 ][ 0 ], gp->directed[ 0 ][ 1 ], gp->directed[ 0 ][ 2 ] );
	#endif

	/* store direction */
	NormalToLatLong( thisdir, bgp->latLong );
}



/*
   SetupGrid()
   calculates the size of the lightgrid and allocates memory
 */

static void SetupGrid(){
	
}



/*
   LightWorld()
   does what it says...
 */

static void LightWorld( bool fastAllocate ){
	
}



/*
   LightMain()
   main routine for light processing
 */

int LightMain( Args& args ){
	

	/* return to sender */
	return 0;
}
