# Sensistory
A simple time-series historian designed for sparse or compressed sensor data

**NOTE: This service is incomplete and not ready for production use.**

Sensistory provides a shard-based, fast recording and fast retrieval time-series database with REST and GRPC APIs.

Unlike many other open-source time-series databases, Sensistory is designed to work well with sparse data

This allows clients to request the condition of a point (or an entire system) at **any time**, rather than simply at those times when specific sensor recordings were made.

## Sparse Data Support

When you perform any query with a time-filter (which will be most queries - Sensistory only works with time-keyed data), the results will include the sample *before* and *after* the time frame. This allows you to trim, interpolate or otherwise modify the outlying points to determine the value during the query times even if there were **no samples** during that query time.

## Compression Support

Sensistory can optionally **compress** your data by avoiding repetitive recording. Due to its sparse data support, the system can ensure that only the *first* and *last* records are retained when one of your channels has a stable value for a long time (ie. discrete channels such as enumerations and boolean values). This may save millions of recorded points in channels that only update occasionally.

## Live/Recent Data Support

Sensistory can deliver *quasi-live* data, delivering the latest values from memory rather than requiring hard-drive access.
