extern crate rusqlite;

use std::env::args_os;
use std::ffi::OsStr;

use rusqlite::Connection;

#[allow(dead_code)]
mod flag_constants {
    /* ratbox-services flags */
    pub const US_FLAGS_SUSPENDED: u32 = 0x0001;
    pub const US_FLAGS_PRIVATE: u32 = 0x0002;
    pub const US_FLAGS_NEVERLOGGEDIN: u32 = 0x0004;
    pub const US_FLAGS_NOACCESS: u32 = 0x0008;
    pub const US_FLAGS_NOMEMOS: u32 = 0x0010;

    pub const NS_FLAGS_WARN: u32 = 0x0001;

    pub const CS_FLAGS_SUSPENDED: u32 = 0x0001;
    pub const CS_FLAGS_NOOPS: u32 = 0x0002;
    pub const CS_FLAGS_AUTOJOIN: u32 = 0x0004;
    pub const CS_FLAGS_WARNOVERRIDE: u32 = 0x0008;
    pub const CS_FLAGS_RESTRICTOPS: u32 = 0x0010;
    pub const CS_FLAGS_NOVOICES: u32 = 0x0020;
    pub const CS_FLAGS_NOVOICECMD: u32 = 0x0040;
    pub const CS_FLAGS_NOUSERBANS: u32 = 0x0080;

    pub const CS_MEMBER_AUTOOP: u32 = 0x001;
    pub const CS_MEMBER_AUTOVOICE: u32 = 0x002;

    /* atheme-services flags */
    pub const CMODE_INVITE: u32 = 0x00000001;
    pub const CMODE_KEY: u32 = 0x00000002;
    pub const CMODE_LIMIT: u32 = 0x00000004;
    pub const CMODE_MOD: u32 = 0x00000008;
    pub const CMODE_NOEXT: u32 = 0x00000010;
    pub const CMODE_PRIV: u32 = 0x00000040;
    pub const CMODE_SEC: u32 = 0x00000080;
    pub const CMODE_TOPIC: u32 = 0x00000100;
}

use flag_constants::*;

fn process(fname: &OsStr) -> rusqlite::Result<()> {
    let conn = Connection::open_with_flags(fname, rusqlite::SQLITE_OPEN_READ_ONLY)?;
    println!("DBV 8");
    println!("CF +AFORVbfiorstv");
    let mut stmt = conn.prepare("SELECT username, password, email, reg_time, last_time, flags FROM users;")?;
    let mut user_rows = stmt.query(&[])?;
    while let Some(user_result) = user_rows.next() {
        let u = user_result?;
        let username: String = u.get(0);
        let password: String = u.get(1);
        let mut email: String = u.get(2);
        let reg_time: i64 = u.get(3);
        let last_time: i64 = u.get(4);
        let rs_flags: u32 = u.get(5);
        if email.is_empty() {
            email.push_str("noemail");
        }
        let mut ath_flags = "C".to_owned();
        if rs_flags & US_FLAGS_PRIVATE != 0 {
            ath_flags.push_str("ps");
        }
        if rs_flags & US_FLAGS_NEVERLOGGEDIN != 0 {
            ath_flags.push('b');
        }
        if rs_flags & US_FLAGS_NOACCESS != 0 {
            ath_flags.push('n');
        }
        if rs_flags & US_FLAGS_NOMEMOS != 0 {
            ath_flags.push('m');
        }
        println!("MU {} {} {} {} {} +{}", username, password, email,
            reg_time, last_time, ath_flags);
    }
    let mut stmt = conn.prepare("SELECT nickname, username, reg_time, last_time FROM nicks;")?;
    let mut nick_rows = stmt.query(&[])?;
    while let Some(nick_result) = nick_rows.next() {
        let n = nick_result?;
        let nickname: String = n.get(0);
        let username: String = n.get(1);
        let reg_time: i64 = n.get(2);
        let last_time: i64 = n.get(3);
        println!("MN {} {} {} {}", username, nickname, reg_time, last_time);
    }
    let mut stmt = conn.prepare("SELECT chname, topic, url, enforcemodes, tsinfo, reg_time, last_time, flags FROM channels;")?;
    let mut chan_rows = stmt.query(&[])?;
    while let Some(chan_result) = chan_rows.next() {
        let c = chan_result?;
        let chname: String = c.get(0);
        let topic: String = c.get(1);
        let url: String = c.get(2);
        let enforcemodes: String = c.get(3);
        let tsinfo: i64 = c.get(4);
        let reg_time: i64 = c.get(5);
        let last_time: i64 = c.get(6);
        let rs_flags: u32 = c.get(7);
        let mut ath_mlock = 0;
        if enforcemodes.contains('i') {
            ath_mlock |= CMODE_INVITE;
        }
        if enforcemodes.contains('m') {
            ath_mlock |= CMODE_MOD;
        }
        if enforcemodes.contains('n') {
            ath_mlock |= CMODE_NOEXT;
        }
        if enforcemodes.contains('p') {
            ath_mlock |= CMODE_PRIV;
        }
        if enforcemodes.contains('s') {
            ath_mlock |= CMODE_SEC;
        }
        if enforcemodes.contains('t') {
            ath_mlock |= CMODE_TOPIC;
        }
        let mut ath_flags = String::new();
        if rs_flags & CS_FLAGS_AUTOJOIN != 0 {
            ath_flags.push('g');
        }
        if rs_flags & CS_FLAGS_WARNOVERRIDE != 0 {
            ath_flags.push('v');
        }
        if rs_flags & CS_FLAGS_RESTRICTOPS != 0 {
            ath_flags.push('z');
        }
        println!("MC {} {} {} +{} {} {} {}", chname, reg_time, last_time,
            ath_flags, ath_mlock, 0, 0);
        if !topic.is_empty() {
            println!("MDC {} private:topic:text {}", chname, topic);
            println!("MDC {} private:topic:setter ChanServ", chname);
            println!("MDC {} private:topic:ts {}", chname, last_time);
        }
        if !url.is_empty() {
            println!("MDC {} url {}", chname, url);
        }
        if tsinfo > 0 {
            println!("MDC {} private:channelts {}", chname, tsinfo);
        }
    }
    let mut stmt = conn.prepare("SELECT chname, username, level, flags, suspend FROM members;")?;
    let mut chanuser_rows = stmt.query(&[])?;
    while let Some(chanuser_result) = chanuser_rows.next() {
        let cu = chanuser_result?;
        let chname: String = cu.get(0);
        let username: String = cu.get(1);
        let rs_level: i32 = cu.get(2);
        let rs_flags: u32 = cu.get(3);
        let mut ath_level = String::new();
        if rs_level >= 1 {
            ath_level.push_str("iv");
        }
        if rs_level >= 10 {
            ath_level.push('r');
        }
        if rs_level >= 50 {
            ath_level.push_str("ot");
        }
        if rs_level >= 100 {
            ath_level.push('A');
        }
        if rs_level >= 140 {
            ath_level.push('R');
        }
        if rs_level >= 150 {
            ath_level.push('f');
        }
        if rs_level >= 190 {
            ath_level.push('s');
        }
        if rs_level >= 200 {
            ath_level.push('F');
        }
        if rs_flags & CS_MEMBER_AUTOVOICE != 0 {
            ath_level.push('V');
        }
        if rs_flags & CS_MEMBER_AUTOOP != 0 {
            ath_level.push('O');
        }
        println!("CA {} {} +{} {}", chname, username, ath_level, 0);
    }
    let mut stmt = conn.prepare("SELECT chname, mask, reason FROM bans;")?;
    let mut chanban_rows = stmt.query(&[])?;
    while let Some(chanban_result) = chanban_rows.next() {
        let cb = chanban_result?;
        let chname: String = cb.get(0);
        let mask: String = cb.get(1);
        let reason: String = cb.get(2);
        println!("CA {} {} +{} {}", chname, mask, "b", 0);
        if !reason.is_empty() {
            println!("MDA {}:{} reason {}", chname, mask, reason);
        }
    }
    Ok(())
}

fn main() {
    let fname = args_os().nth(1).expect("need a db pathname");
    process(&fname).unwrap();
}
