package leemonaya

import cats.effect.{IO, Sync}
import io.circe.Json
import org.http4s.HttpRoutes
import org.http4s.circe.{jsonEncoder, jsonOf}
import org.http4s.dsl.Http4sDsl
import io.circe.generic.auto._
import org.http4s.circe.CirceEntityEncoder.circeEntityEncoder

case class StationFeedMsg(stationId: String, temperature: Double, humidity: Double)

class ServerRoutes[F[_]: Sync] extends Http4sDsl[F] {

  implicit val stationFeedMsgJsonDecoder = jsonOf[F, StationFeedMsg]

  val routes: HttpRoutes[F] =
    HttpRoutes.of[F] {
      case GET -> Root / "hello" / name =>
        Ok("ciao")
      case POST -> Root / "station-feed-working" =>
        //Ok(Json.obj("message" -> Json.fromString(s"Hello")))
        NoContent()
      case req @ POST -> Root / "station-feed" =>
        Ok(req.as[StationFeedMsg])

      case req @ POST -> Root / "station-feed2" =>
        req.decode[StationFeedMsg] { m =>
          Ok(m)
        }
    }
}

