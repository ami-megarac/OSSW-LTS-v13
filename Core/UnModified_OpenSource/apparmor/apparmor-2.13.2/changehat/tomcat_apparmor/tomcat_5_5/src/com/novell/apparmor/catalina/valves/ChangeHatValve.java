/* ------------------------------------------------------------------
 *
 *    Copyright (C) 2002-2007 Novell/SUSE
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of version 2 of the GNU General Public
 *    License published by the Free Software Foundation.
 *
 * ------------------------------------------------------------------ */

package com.novell.apparmor.catalina.valves;

import com.novell.apparmor.JNIChangeHat;
import java.io.IOException;
import javax.servlet.ServletException;
import org.apache.catalina.Container;
import org.apache.catalina.valves.ValveBase;
import java.security.SecureRandom;


public final class ChangeHatValve extends ValveBase {
    // JNI interface class for AppArmor change_hat
    private static JNIChangeHat changehat_wrapper = new JNIChangeHat();
    private static SecureRandom randomNumberGenerator = null;
    private static String DEFAULT_HAT = "DEFAULT";
    private static int SERVLET_PATH_MEDIATION = 0;
    private static int URI_MEDIATION = 1;

    private int mediationType = ChangeHatValve.SERVLET_PATH_MEDIATION;

    /*
     *
     *  Property setter called during the parsing of the server.xml.
     *  If the <code>mediationType</code> is an attribute of the
     *  Valve definition for
     *  <code>com.novell.apparmor.catalina.valves.ChangeHatValve</code>
     *  then this setter will be called to set the value for this property.
     *
     *  @param type <b>URI|ServletPath<b>
     *
     *  Controls what granularity of security confinement when used with
     *  AppArmor change_hat(2). Either based upon <code>getServletPath()</code>
     *  or  <code>getRequestURI()</code> called against on the request.
     *
     */
    public void setMediationType( String type ) {
        if ( type.equalsIgnoreCase("URI") ) {
            this.mediationType = ChangeHatValve.URI_MEDIATION;
        } else if ( type.equalsIgnoreCase("servletPath") ) {
            this.mediationType = ChangeHatValve.SERVLET_PATH_MEDIATION;
        }
    }

    /*
     *
     * Return an int value representing the currently configured
     * <code>mediationType</code> for this instance.
     *
     */
    public int getMediationType() {
        return this.mediationType;
    }


    /*
     *
     * Return an instance of <code>SecureRandom</code> creating one if necessary
     *
     */
    SecureRandom getRndGen() {
        if ( ChangeHatValve.randomNumberGenerator == null) {
            ChangeHatValve.randomNumberGenerator =  new java.security.SecureRandom();
        }
        return ChangeHatValve.randomNumberGenerator;
    }

    /*
     *
     * Call to return a random cookie from the <code>SecureRandom</code> PRNG
     *
     */
    int getCookie() {
        SecureRandom rnd = getRndGen();
        if ( rnd == null ) {
            this.getContainer().getLogger().error(
                "[APPARMOR] can't initialize SecureRandom for cookie" +
                " generation for change_hat() call.");
            return 0;
        }
        return rnd.nextInt();
    }


    /*
     *
     * Call out to AppArmor change_hat(2) to change the security
     * context for the processing of the request by subsequent valves.
     * Returns to the current security context when processing is complete.
     * The security context that is chosen is govern by the
     * <code>mediationType</code> property - which can be set in the
     * <code>server.xml</code> file.
     *
     * @param request Request being processed
     * @param response Response being processed
     * @param context The valve context used to invoke the next valve
     *  in the current processing pipeline
     *
     * @exception IOException if an input/output error has occurred
     * @exception ServletException if a servlet error has occurred
     *
     */
    public void invoke( org.apache.catalina.connector.Request request,
                        org.apache.catalina.connector.Response response )
    throws IOException, ServletException {

        Container container = this.getContainer();
        int cookie, result;
        boolean inSubHat = false;

        container.getLogger().debug(this.getClass().toString() +
            "[APPARMOR] Request received [" + request.getInfo()
            + "]");

        String hatname = ChangeHatValve.DEFAULT_HAT;;
        if ( getMediationType() == ChangeHatValve.SERVLET_PATH_MEDIATION ) {
            hatname = request.getServletPath();
        } else if ( getMediationType() == ChangeHatValve.URI_MEDIATION ) {
            hatname = request.getRequestURI();
        }

       /*
        * Select the AppArmor container for this request:
        *
        *  1. try hat name from either URI or ServletPath
        *     (based on configuration)
        *
        *  2. try hat name of the defined DEFAULT_HAT
        *
        *  3. run in the current AppArmor context
        */

        cookie = getCookie();
        if ( hatname == null || "".equals(hatname) ) {
            hatname = ChangeHatValve.DEFAULT_HAT;
        }
        container.getLogger().debug("[APPARMOR] ChangeHat to [" + hatname
          + "] cookie [" + cookie + "]");

        result = changehat_wrapper.changehat_in(hatname, cookie);

        if ( result == JNIChangeHat.EPERM ) {
            container.getLogger().error("[APPARMOR] change_hat valve " +
              "configured but Tomcat process is not confined by an " +
              "AppArmor profile.");
            getNext().invoke(request, response);
        } else {
            if ( result == JNIChangeHat.EACCES ) {
            changehat_wrapper.changehat_out(cookie);
            result = changehat_wrapper.changehat_in(ChangeHatValve.DEFAULT_HAT,
                                                    cookie);
            if ( result != 0 ) {
                changehat_wrapper.changehat_out(cookie);
                container.getLogger().error("[APPARMOR] ChangeHat to [" + hatname
                  + "] failed. Running in parent context.");
            } else  {
                inSubHat = true;
            }
        } else if ( result != 0 ) {
            changehat_wrapper.changehat_out(cookie);
            container.getLogger().error("[APPARMOR] ChangeHat to [" + hatname
              + "] failed. Running in parent context.");
        } else {
            inSubHat = true;
        }
        getNext().invoke(request, response);
        if ( inSubHat ) changehat_wrapper.changehat_out(cookie);
        }
    }
}

